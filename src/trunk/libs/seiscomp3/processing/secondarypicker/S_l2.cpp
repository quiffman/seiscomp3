/***************************************************************************
 *   Copyright (C) by GFZ Potsdam                                          *
 *                                                                         *
 *   You can redistribute and/or modify this program under the             *
 *   terms of the SeisComP Public License.                                 *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   SeisComP Public License for more details.                             *
 ***************************************************************************/


#define SEISCOMP_COMPONENT L2Picker

#include <seiscomp3/logging/log.h>
#include <seiscomp3/processing/operator/ncomps.h>
#include <seiscomp3/processing/operator/l2norm.h>
//#include <seiscomp3/io/records/mseedrecord.h>
#include <seiscomp3/math/filter.h>

#include "S_l2.h"


using namespace std;

namespace Seiscomp {

namespace Processing {

namespace {

string MethodID = "L2-AIC";


template <typename T, int N, class PROC>
class FilterWrapper {
	public:
		FilterWrapper(Math::Filtering::InPlaceFilter<T> *filter,
		              const PROC &proc)
		: _proc(proc), _baseFilter(filter) {
			for ( int i = 0; i < N; ++i ) _filter[i] = NULL;
		}

		~FilterWrapper() {
			for ( int i = 0; i < N; ++i )
				if ( _filter[i] ) delete _filter[i];
		}

		void operator()(T *data[N], int n, const Core::Time &stime, double sfreq) const {
			if ( _baseFilter ) {
				for ( int i = 0; i < N; ++i ) {
					if ( _filter[i] == NULL ) {
						_filter[i] = _baseFilter->clone();
						_filter[i]->setSamplingFrequency(sfreq);
					}

					_filter[i]->apply(n, data[i]);
				}
			}

			// Call real operator
			_proc(data, n, stime, sfreq);
		}

		bool publish(int c) const { return _proc.publish(c); }

		int compIndex(const string &code) const {
			return _proc.compIndex(code);
		}


	private:
		PROC _proc;
		Math::Filtering::InPlaceFilter<T> *_baseFilter;
		mutable Math::Filtering::InPlaceFilter<T> *_filter[N];
};


template<typename TYPE>
void
maeda_aic(int n, const TYPE *data, int &kmin, double &snr, int margin=10) {
	// expects a properly filtered and demeaned trace

	// windowed sum for variance computation
	double sumwin1 = 0, sumwin2 = 0, minaic = 0;
	int imin = margin, imax = n-margin;
	for ( int i = 0; i < n; ++i ) {
		TYPE squared = data[i]*data[i];
		if ( i < imin )
			sumwin1 += squared;
		else
			sumwin2 += squared;
	}

	for ( int k = imin; k < imax; ++k ) {
		double var1 = sumwin1/(k-1),
		       var2 = sumwin2/(n-k-1);
		double aic = k*log10(var1) + (n-k-1)*log10(var2);
		TYPE squared = data[k]*data[k];

		sumwin1 += squared;
		sumwin2 -= squared;

		if ( k == imin ) minaic = aic;
		if ( aic < minaic ) {
			minaic = aic;
			kmin = k;
			snr = var2/var1;
		}
	}
}


}


REGISTER_SECONDARYPICKPROCESSOR(SL2Picker, "S-L2");
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SL2Picker::SL2Picker() {
	// Request all three components
	setUsedComponent(Horizontal);
	// Check for S picks one minute after P
	setSignalStart(0);
	setSignalEnd(60);
	setMargin(Core::TimeSpan(0,0));
	_threshold = 3;
	_timeCorr = -0.4;
	_minSNR = 15;
	_margin = 5.0;
	_initialized = false;
	_compFilter = NULL;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SL2Picker::~SL2Picker() {
	if ( _compFilter ) delete _compFilter;

	/*
	if ( lastRecord() ) {
		GenericRecord rec(lastRecord()->networkCode(),
		                  lastRecord()->stationCode(),
		                  "L2",
		                  lastRecord()->channelCode(),
		                  dataTimeWindow().startTime(), lastRecord()->samplingFrequency());
		ArrayPtr data = continuousData().clone();
		rec.setData(data.get());
		IO::MSeedRecord mseed(rec);
		mseed.write(cout);
	}
	*/
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool SL2Picker::setup(const Settings &settings) {
	if ( !SecondaryPicker::setup(settings) ) return false;

	// Check all three components for valid gains and orientations
	for ( int i = 1; i < 3; ++i ) {
		if ( _streamConfig[i].code().empty() ) {
			SEISCOMP_ERROR("[S-L2] component[%d] code is empty", i);
			setStatus(Error, i);
			return false;
		}

		if ( _streamConfig[i].gain == 0.0 ) {
			SEISCOMP_ERROR("[S-L2] component[%d] gain is missing", i);
			setStatus(MissingGain, i);
			return false;
		}
	}

	try { setSignalStart(settings.getDouble("spicker.L2.signalBegin")); }
	catch ( ... ) {}

	try { setSignalEnd(settings.getDouble("spicker.L2.signalEnd")); }
	catch ( ... ) {}

	try { _threshold = settings.getDouble("spicker.L2.threshold"); }
	catch ( ... ) {}

	try { _minSNR = settings.getDouble("spicker.L2.minSNR"); }
	catch ( ... ) {}

	try { _margin = settings.getDouble("spicker.L2.marginAIC"); }
	catch ( ... ) {}

	try { _timeCorr = settings.getDouble("spicker.L2.timeCorr"); }
	catch ( ... ) {}

	string fg;

	try { fg = settings.getString("spicker.L2.filter"); }
	catch ( ... ) { fg = "BW(4,0.3,1.0)"; }

	if ( !fg.empty() ) {
		_compFilter = Filter::Create(fg.c_str());
		if ( _compFilter == NULL ) {
			SEISCOMP_WARNING("L2 spicker: wrong component filter definition: %s",
			                 fg.c_str());
			return false;
		}
	}

	try { fg = settings.getString("spicker.L2.detecFilter"); }
	catch ( ... ) { fg = "STALTA(1,10)"; }

	if ( !fg.empty() ) {
		Filter *filter = Filter::Create(fg.c_str());
		if ( filter == NULL ) {
			SEISCOMP_WARNING("L2 spicker: wrong filter definition: %s",
			                 fg.c_str());
			return false;
		}

		setFilter(filter);
	}

	// Create a waveform operator that combines all three channels and
	// computes the l2norm of each 3 component sample
	typedef Operator::StreamConfigWrapper<double,2,Operator::L2Norm> OpWrapper;
	typedef FilterWrapper<double,2,OpWrapper> FilterL2Norm;
	typedef NCompsOperator<double,2,FilterL2Norm> L2Norm;

	WaveformOperatorPtr op = new L2Norm(FilterL2Norm(_compFilter, OpWrapper(_streamConfig+1, Operator::L2Norm<double,2>())));
	setOperator(op.get());

	_initialized = true;

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
const string &SL2Picker::methodID() const {
	return MethodID;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void SL2Picker::fill(size_t n, double *samples) {
	// Disable dectection filter since we need the frequency filtered data.
	// The detection filter is applied during process
	Filter *tmp = _stream.filter;
	_stream.filter = NULL;
	SecondaryPicker::fill(n, samples);
	// Restore detection filter
	_stream.filter = tmp;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void SL2Picker::process(const Record *rec, const DoubleArray &filteredData) {
	/*
	IO::MSeedRecord mseed(*rec);
	mseed.setLocationCode("L2");
	mseed.write(cout);
	*/

	if ( !_initialized ) return;

	// The result of the L2 norm operator is stored in the vertical component
	// and already sensitivity corrected.
	size_t n = filteredData.size();
	int si = 0;

	if ( rec->startTime() < _trigger.onset )
		si = (int)(double(_trigger.onset - rec->startTime()) * rec->samplingFrequency());

	double progress = (_config.signalEnd - _config.signalBegin) / double(dataTimeWindow().endTime() - _trigger.onset);
	if ( progress < 0 ) progress = 0;
	else if ( progress > 100 ) progress = 100;

	setStatus(InProgress, progress);

	// Active trigger but AIC time window not filled during last call?
	if ( _result.time.valid() && _margin > 0 ) {
		int kmin;
		double snr;
		int ai = (int)(double(_result.time - dataTimeWindow().startTime()) * _stream.fsamp);
		ai -= (int)(_margin*_stream.fsamp);
		int ae = ai + (int)(_margin*2*_stream.fsamp);
		bool skip = ae > continuousData().size();

		if ( skip ) return;

		maeda_aic(ae-ai, continuousData().typedData()+ai, kmin, snr);

		double t = (double)(kmin+ai)/(double)continuousData().size();
		Core::Time tm = dataTimeWindow().startTime() + Core::TimeSpan(t*dataTimeWindow().length());
		_result.time = tm + Core::TimeSpan(_timeCorr);

		if ( snr < _minSNR ) {
			_initialized = false;
			SEISCOMP_DEBUG("[S-L2] %s: snr %f too low at %s, need %f",
			               rec->streamID().c_str(), snr,
			               _result.time.iso().c_str(),
			               _minSNR);
			setStatus(LowSNR, snr);
			_result = Result();
			return;
		}

		if ( _result.time <= _trigger.onset ) {
			_initialized = false;
			SEISCOMP_DEBUG("[S-L2] %s: pick at %s is before trigger at %s: rejected",
			               rec->streamID().c_str(),
			               _result.time.iso().c_str(),
			               _trigger.onset.iso().c_str());
			setStatus(Terminated, 1);
			_result = Result();
			return;
		}

		_result.snr = snr;
	}

	size_t idx = si;

	// If no pick has been found, check the data
	if ( !_result.time.valid() ) {
		for ( ; idx < n; ++idx ) {
			double v = filteredData[idx];
			double fv(v);

			// Apply detection filter
			if ( _stream.filter ) _stream.filter->apply(1, &fv);

			// Trigger detected
			if ( fv >= _threshold ) {
				double t = (double)idx/(double)n;
				_result.time = rec->startTime() + Core::TimeSpan(rec->timeWindow().length() * t);
				_result.timeLowerUncertainty = _result.timeUpperUncertainty = -1;
				_result.snr = -1;

				if ( _margin > 0 ) {
					int kmin;
					double snr;
					int ai = (int)(double(_result.time - dataTimeWindow().startTime()) * _stream.fsamp);
					ai -= (int)(_margin*_stream.fsamp);
					int ae = ai + (int)(_margin*2*_stream.fsamp);
					if ( ae > continuousData().size() )
						return;

					maeda_aic(ae-ai, continuousData().typedData()+ai, kmin, snr);

					t = (double)(kmin+ai)/(double)continuousData().size();
					Core::Time tm = dataTimeWindow().startTime() + Core::TimeSpan(t*dataTimeWindow().length());
					_result.time = tm + Core::TimeSpan(_timeCorr);

					if ( snr < _minSNR ) {
						_result = Result();
						_initialized = false;
						SEISCOMP_DEBUG("[S-L2] %s: snr %f too low at %s, need %f",
						               rec->streamID().c_str(), snr,
						               _result.time.iso().c_str(),
						               _minSNR);
						setStatus(LowSNR, snr);
						return;
					}

					_result.snr = snr;
				}
				else
					_result.time +=  Core::TimeSpan(_timeCorr);

				if ( _result.time <= _trigger.onset ) {
					_result = Result();
					_initialized = false;
					SEISCOMP_DEBUG("[S-L2] %s: pick at %s is before trigger at %s: rejected",
					               rec->streamID().c_str(),
					               _result.time.iso().c_str(),
					               _trigger.onset.iso().c_str());
					setStatus(Terminated, 1);
					return;
				}

				break;
			}
		}
	}

	// Time still valid, emit pick
	if ( _result.time.valid() ) {
		_result.phaseCode = "S";
		_result.record = rec;
		SEISCOMP_DEBUG("[S-L2] %s: %s pick at %s with snr=%f",
		               rec->streamID().c_str(), _result.phaseCode.c_str(),
		               _result.time.iso().c_str(),
		               _result.snr);
		setStatus(Finished, 100);
		_initialized = false;
		emitPick(_result);

		// Reset result
		_result = Result();

		return;
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
