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


#define SEISCOMP_COMPONENT WaveformProcessor

#include <seiscomp3/processing/waveformprocessor.h>
#include <seiscomp3/logging/log.h>


namespace Seiscomp {

namespace Processing {

IMPLEMENT_SC_ABSTRACT_CLASS(WaveformProcessor, "WaveformProcessor");
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
WaveformProcessor::WaveformProcessor(const Core::TimeSpan &initTime,
                                     const Core::TimeSpan &gapThreshold)
 : _filter(NULL), _enabled(true),
   _initTime(initTime), _gapThreshold(gapThreshold), _usedComponent(Vertical) {

	_gapTolerance = 0.;
	_enableGapInterpolation = false;
	reset();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
WaveformProcessor::~WaveformProcessor() {
	close();
	if ( _filter ) delete _filter;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Stream &WaveformProcessor::streamConfig(Component c) {
	return _streamConfig[c];
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
const Stream &WaveformProcessor::streamConfig(Component c) const {
	return _streamConfig[c];
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void WaveformProcessor::setFilter(Filter *filter) {
	if ( _filter ) delete _filter;
	_filter = filter;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void WaveformProcessor::setGapTolerance(const Core::TimeSpan &length) {
	_gapTolerance = length;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
const Core::TimeSpan &WaveformProcessor::gapTolerance() const {
	return _gapTolerance;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void WaveformProcessor::reset() {
	if ( _filter )
		setFilter(_filter->clone());

	_lastRecord = NULL;
	_fsamp = 0.0;
	_dataTimeWindow = Core::TimeWindow();
	_lastSampleTime = Core::Time();
	_lastSample = 0;

	_receivedSamples = 0;
	_neededSamples = 0;

	_status = WaitingForData;
	_statusValue = 0.;

	_initialized = false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void WaveformProcessor::setEnabled(bool e) {
	if ( _enabled == e ) return;
	_enabled = e;
	_enabled?enabled():disabled();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void WaveformProcessor::setGapInterpolationEnabled(bool enable) {
	_enableGapInterpolation = enable;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool WaveformProcessor::isGapInterpolationEnabled() const {
	return _enableGapInterpolation;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Core::TimeWindow WaveformProcessor::dataTimeWindow() const {
	return _dataTimeWindow;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
const Record *WaveformProcessor::lastRecord() const {
	return _lastRecord.get();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void WaveformProcessor::initFilter(double fsamp) {
	_fsamp = fsamp;
	_neededSamples = static_cast<size_t>(_initTime * _fsamp + 0.5);
	if ( _filter ) _filter->setSamplingFrequency(fsamp);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
double WaveformProcessor::samplingFrequency() const {
	return _fsamp;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool WaveformProcessor::feed(const Record *record) {
	if ( _status > InProgress ) return false;
	if ( record->data() == NULL ) return false;

	DoubleArrayPtr arr = (DoubleArray*)record->data()->copy(Array::DOUBLE);

	if ( _lastRecord ) {
		if ( record == _lastRecord ) return false;

		Core::TimeSpan gap = record->startTime() - _lastSampleTime - Core::TimeSpan(0,1);
		double gapSecs = (double)gap;

		if ( gap > _gapThreshold ) {
			size_t gapsize = static_cast<size_t>(ceil(_fsamp * gapSecs));
			bool handled = handleGap(_filter, gap, _lastSample, (*arr)[0], gapsize);
			if ( handled )
				SEISCOMP_DEBUG("[%s] detected gap of %.6f secs or %lu samples (handled)",
				               record->streamID().c_str(), (double)gap, (unsigned long)gapsize);
			else {
				SEISCOMP_DEBUG("[%s] detected gap of %.6f secs or %lu samples (NOT handled): status = %s",
				               record->streamID().c_str(), (double)gap, (unsigned long)gapsize,
				               status().toString());
				if ( _status > InProgress ) return false;
			}
		}
		else if ( gapSecs < 0 ) {
			size_t gapsize = static_cast<size_t>(ceil(-_fsamp * gapSecs));
			if ( gapsize > 1 ) return false;
		}

		// update the received data timewindow
		_dataTimeWindow.setEndTime(record->endTime());
	}

	// NOTE: Do not use else here, because lastRecord can be set NULL
	//       when calling reset() in handleGap(...)
	if ( !_lastRecord ) {
		initFilter(record->samplingFrequency());

		// update the received data timewindow
		_dataTimeWindow = record->timeWindow();
		/*
		std::cerr << "Received first record for " << record->streamID() << ", "
		          << className() << " [" << record->startTime().iso() << " - " << record->endTime().iso() << std::endl;
		*/
	}

	// Fill the values and do the actual filtering
	fill(arr->size(), arr->typedData());

	if ( !_initialized ) {
		if ( _receivedSamples > _neededSamples ) {
			//_initialized = true;
			process(record, *arr);
			// NOTE: To allow derived classes to notice modification of the variable 
			//       _initialized, it is necessary to set this after calling process.
			_initialized = true;
		}
	}
	else
		// Call process to cause a derived processor to work on the data.
		process(record, *arr);

	_lastRecord = record;

	_lastSampleTime = record->endTime();
	_lastSample = (*arr)[arr->size()-1];

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int WaveformProcessor::feedSequence(const RecordSequence *sequence) {
	int count = 0;

	if ( sequence == NULL ) return count;
	for ( RecordSequence::const_iterator it = sequence->begin();
	      it != sequence->end(); ++it )
		if ( feed(it->get()) ) ++count;

	return count;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void WaveformProcessor::setUserData(Core::BaseObject *obj) const {
	_userData = obj;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Core::BaseObject *WaveformProcessor::userData() const {
	return _userData.get();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void WaveformProcessor::fill(size_t n, double *samples) {
	_receivedSamples += n;
	if ( _filter ) _filter->apply(n, samples);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool WaveformProcessor::handleGap(Filter *filter, const Core::TimeSpan& span,
                                  double lastSample, double nextSample,
                                  size_t missingSamples) {
	if ( span <= _gapTolerance ) {
		if ( _enableGapInterpolation ) {
			double delta = nextSample - lastSample;
			double step = 1./(double)(missingSamples+1);
			double di = step;
			for ( size_t i = 0; i < missingSamples; ++i, di += step ) {
				double value = lastSample + di*delta;
				fill(1, &value);
			}
		}

		return true;
	}

	return false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool WaveformProcessor::isFinished() const {
	return _status > InProgress;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void WaveformProcessor::setStatus(Status status, double value) {
	_status = status;
	_statusValue = value;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void WaveformProcessor::terminate() {
	setStatus(Terminated, _status);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
WaveformProcessor::Status WaveformProcessor::status() const {
	return _status;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
double WaveformProcessor::statusValue() const {
	return _statusValue;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void WaveformProcessor::close() const {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}

}
