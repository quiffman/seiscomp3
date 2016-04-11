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



#include <seiscomp3/processing/amplitudeprocessor_ML.h>
#include <seiscomp3/math/filter/seismometers.h>

#include <boost/bind.hpp>
#include <cstdio>


namespace Seiscomp {
namespace Processing {


IMPLEMENT_SC_CLASS_DERIVED(AmplitudeProcessor_ML, AmplitudeProcessor, "AmplitudeProcessor_ML");
REGISTER_AMPLITUDEPROCESSOR(AmplitudeProcessor_ML, "ML");


AmplitudeProcessor_MLh::AmplitudeProcessor_MLh()
{
	_type = "ML";
}


AmplitudeProcessor_ML::AmplitudeProcessor_ML()
 : Processing::AmplitudeProcessor("ML") {
	setSignalEnd(150.);
	setMinSNR(0);
	// Maximum distance is 8 degrees
	setMaxDist(8);
	// Maximum depth is 80 km
	setMaxDepth(80);

	setUsedComponent(Horizontal);

	_combiner = TakeAverage;

	_ampN.setUsedComponent(FirstHorizontal);
	_ampE.setUsedComponent(SecondHorizontal);

	_ampE.setPublishFunction(boost::bind(&AmplitudeProcessor_ML::newAmplitude, this, _1, _2));
	_ampN.setPublishFunction(boost::bind(&AmplitudeProcessor_ML::newAmplitude, this, _1, _2));
}


const AmplitudeProcessor *AmplitudeProcessor_ML::componentProcessor(Component comp) const {
	switch ( comp ) {
		case FirstHorizontalComponent:
			return &_ampN;
		case SecondHorizontalComponent:
			return &_ampE;
		default:
			break;
	}

	return NULL;
}

const DoubleArray *AmplitudeProcessor_ML::processedData(Component comp) const {
	switch ( comp ) {
		case FirstHorizontalComponent:
			return _ampN.processedData(comp);
		case SecondHorizontalComponent:
			return _ampE.processedData(comp);
		default:
			break;
	}

	return NULL;
}


void AmplitudeProcessor_ML::reprocess(OPT(double) searchBegin, OPT(double) searchEnd) {
	setStatus(WaitingForData, 0);
	_ampN.setConfig(config());
	_ampE.setConfig(config());

	_amplitudes[0] = _amplitudes[1] = Core::None;
	_times[0] = _times[1] = Core::Time();

	_ampN.reprocess(searchBegin, searchEnd);
	_ampE.reprocess(searchBegin, searchEnd);

	if ( !isFinished() ) {
		if ( _ampN.status() > Finished )
			setStatus(_ampN.status(), _ampN.statusValue());
		else
			setStatus(_ampE.status(), _ampE.statusValue());
	}
}


int AmplitudeProcessor_ML::capabilities() const {
	return _ampN.capabilities() | Combiner;
}


AmplitudeProcessor::IDList
AmplitudeProcessor_ML::capabilityParameters(Capability cap) const {
	if ( cap == Combiner ) {
		IDList params;
		params.push_back("Average");
		params.push_back("Max");
		params.push_back("Min");
		return params;
	}

	return _ampN.capabilityParameters(cap);
}


bool AmplitudeProcessor_ML::setParameter(Capability cap, const std::string &value) {
	if ( cap == Combiner ) {
		if ( value == "Min" ) {
			_combiner = TakeMin;
			return true;
		}
		else if ( value == "Max" ) {
			_combiner = TakeMax;
			return true;
		}
		else if ( value == "Average" ) {
			_combiner = TakeAverage;
			return true;
		}

		return false;
	}

	_ampN.setParameter(cap, value);
	return _ampE.setParameter(cap, value);
}


bool AmplitudeProcessor_ML::setup(const Settings &settings) {
	// Copy the stream configurations (gain, orientation, responses, ...) to
	// the horizontal processors
	_ampN.streamConfig(FirstHorizontalComponent) = streamConfig(FirstHorizontalComponent);
	_ampE.streamConfig(SecondHorizontalComponent) = streamConfig(SecondHorizontalComponent);

	if ( !AmplitudeProcessor::setup(settings) ) return false;

	// Setup each component
	if ( !_ampN.setup(settings) || !_ampE.setup(settings) ) return false;

	return true;
}


void AmplitudeProcessor_ML::setTrigger(const Core::Time& trigger) throw(Core::ValueException) {
	AmplitudeProcessor::setTrigger(trigger);
	_ampE.setTrigger(trigger);
	_ampN.setTrigger(trigger);
}


void AmplitudeProcessor_ML::computeTimeWindow() {
	// Copy configuration to each component
	_ampN.setConfig(config());
	_ampE.setConfig(config());

	_ampE.computeTimeWindow();
	_ampN.computeTimeWindow();
	setTimeWindow(_ampE.timeWindow() | _ampN.timeWindow());
}


double AmplitudeProcessor_ML::timeWindowLength(double distance_deg) const {
	double endN = _ampN.timeWindowLength(distance_deg);
	double endE = _ampE.timeWindowLength(distance_deg);
	_ampN.setSignalEnd(endN);
	_ampE.setSignalEnd(endE);
	return std::max(endN, endE);
}


void AmplitudeProcessor_ML::reset() {
	AmplitudeProcessor::reset();

	_amplitudes[0] = _amplitudes[1] = Core::None;
	_times[0] = _times[1] = Core::Time();

	_ampE.reset();
	_ampN.reset();
}


void AmplitudeProcessor_ML::close() {
	// TODO: Check for best available amplitude here
}


bool AmplitudeProcessor_ML::feed(const Record *record) {
	// Both processors finished already?
	if ( _ampE.isFinished() && _ampN.isFinished() ) return false;

	// Did an error occur?
	if ( status() > WaveformProcessor::Finished ) return false;

	if ( record->channelCode() == _streamConfig[FirstHorizontalComponent].code() ) {
		if ( !_ampN.isFinished() ) {
			_ampN.feed(record);
			if ( _ampN.status() == InProgress )
				setStatus(WaveformProcessor::InProgress, _ampN.statusValue());
			else if ( _ampN.isFinished() && _ampE.isFinished() ) {
				if ( !isFinished() ) {
					if ( _ampE.status() == Finished )
						setStatus(_ampN.status(), _ampN.statusValue());
					else
						setStatus(_ampE.status(), _ampE.statusValue());
				}
			}
		}
	}
	else if ( record->channelCode() == _streamConfig[SecondHorizontalComponent].code() ) {
		if ( !_ampE.isFinished() ) {
			_ampE.feed(record);
			if ( _ampE.status() == InProgress )
				setStatus(WaveformProcessor::InProgress, _ampE.statusValue());
			else if ( _ampE.isFinished() && _ampN.isFinished() ) {
				if ( !isFinished() ) {
					if ( _ampN.status() == Finished )
						setStatus(_ampE.status(), _ampE.statusValue());
					else
						setStatus(_ampN.status(), _ampN.statusValue());
				}
			}
		}
	}

	return true;
}


bool AmplitudeProcessor_ML::computeAmplitude(const DoubleArray &data,
                                             size_t i1, size_t i2,
                                             size_t si1, size_t si2,
                                             double offset, double *dt,
                                             double *amplitude, double *width,
                                             double *period, double *snr) {
	return false;
}


void AmplitudeProcessor_ML::newAmplitude(const AmplitudeProcessor *proc,
                                         const AmplitudeProcessor::Result &res) {

	if ( isFinished() ) return;

	if ( !_amplitudes[0] && !_amplitudes[1] ) {
		_timeWindowBegin = res.timeWindowBegin;
		_timeWindowEnd = res.timeWindowEnd;
	}

	int idx = 0;

	if ( proc == &_ampE ) {
		idx = 0;
	}
	else if ( proc == &_ampN ) {
		idx = 1;
	}

	if ( res.timeWindowBegin < _timeWindowBegin )
		_timeWindowBegin = res.timeWindowBegin;

	if ( res.timeWindowEnd < _timeWindowEnd )
		_timeWindowEnd = res.timeWindowEnd;

	_amplitudes[idx] = res.amplitude;
	_amplitudeWidths[idx] = res.amplitudeWidth;
	_times[idx] = res.time;

	if ( _amplitudes[0] && _amplitudes[1] ) {
		setStatus(Finished, 100.);
		Result newRes;
		newRes.record = res.record;

		switch ( _combiner ) {
			case TakeAverage:
				newRes.amplitude = (*_amplitudes[0] + *_amplitudes[1]) * 0.5;
				newRes.time = Core::Time((double(_times[0]) + double(_times[1])) / 2);
				newRes.component = Horizontal;
				newRes.amplitudeWidth = fabs(double(newRes.time-_times[0]));
				break;
			case TakeMin:
				if ( *_amplitudes[0] < *_amplitudes[1] ) {
					newRes.amplitude = *_amplitudes[0];
					newRes.amplitudeWidth = _amplitudeWidths[0];
					newRes.time = double(_times[0]);
					newRes.component = _ampE.usedComponent();
				}
				else {
					newRes.amplitude = *_amplitudes[1];
					newRes.amplitudeWidth = _amplitudeWidths[1];
					newRes.time = double(_times[1]);
					newRes.component = _ampN.usedComponent();
				}
				break;
			case TakeMax:
				if ( *_amplitudes[0] > *_amplitudes[1] ) {
					newRes.amplitude = *_amplitudes[0];
					newRes.amplitudeWidth = _amplitudeWidths[0];
					newRes.time = double(_times[0]);
					newRes.component = _ampE.usedComponent();
				}
				else {
					newRes.amplitude = *_amplitudes[1];
					newRes.amplitudeWidth = _amplitudeWidths[1];
					newRes.time = double(_times[1]);
					newRes.component = _ampN.usedComponent();
				}
				break;
		};

		newRes.period = -1;
		newRes.snr = -1;
		newRes.timeWindowBegin = _timeWindowBegin;
		newRes.timeWindowEnd = _timeWindowEnd;
		emitAmplitude(newRes);
	}
}


}
}
