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


#define SEISCOMP_COMPONENT Picker

#include <seiscomp3/processing/picker.h>
#include <seiscomp3/math/mean.h>
#include <seiscomp3/core/interfacefactory.ipp>
#include <seiscomp3/logging/log.h>

#include <fstream>


IMPLEMENT_INTERFACE_FACTORY(Seiscomp::Processing::Picker, SC_CORE_PROCESSING_API);


namespace Seiscomp {
namespace Processing {

IMPLEMENT_SC_ABSTRACT_CLASS_DERIVED(Picker, TimeWindowProcessor, "Picker");
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Picker::Picker() {
	init();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Picker::Picker(const Core::Time& trigger)
 : _trigger(trigger) {
	init();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Picker::init() {
	setMargin(Core::TimeSpan(0.0));
	_config.signalBegin = -30;
	_config.signalEnd   =  10;
	_config.snrMin      =   3;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Picker::~Picker() {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Picker::computeTimeWindow() {
	if ( !(bool)_trigger ) {
		setTimeWindow(Core::TimeWindow());
		return;
	}

	Core::Time startTime = _trigger + Core::TimeSpan(_config.signalBegin);
	Core::Time   endTime = _trigger + Core::TimeSpan(_config.signalEnd);

	setTimeWindow(Core::TimeWindow(startTime, endTime));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Picker::handleGap(Filter *filter, const Core::TimeSpan& span,
                       double lastSample, double nextSample,
                       size_t missingSamples) {
	setStatus(QCError, -1);
	terminate();

	//TODO: Handle gaps
	return false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Picker::reset() {
	TimeWindowProcessor::reset();
	_trigger = Core::Time();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Picker::setTrigger(const Core::Time& trigger) throw(Core::ValueException) {
	if ( _trigger )
		throw Core::ValueException("The trigger has been set already");

	_trigger = trigger;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Picker::setPublishFunction(const PublishFunc& func) {
	_func = func;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Picker::emitPick(const Result &result) {
	if ( isEnabled() && _func )
		_func(this, result);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Picker::calculatePick(int n, const double *data,
                           int signalStartIdx, int signalEndIdx,
                           int &triggerIdx, int &lowerUncertainty,
                           int &upperUncertainty, double &snr) {
	return false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Picker::process(const Record *record, const DoubleArray &) {
	// Sampling frequency has not been set yet
	if ( _fsamp == 0.0 ) {
		terminate();
		return;
	}

	// Error occured while feeding a record or already finished?
	if ( isFinished() ) return;

	// Process unless the available time window does not include the
	// requested time window
	if ( !dataTimeWindow().contains(timeWindow())) return;

	// data window relative to continuous()->startTime()
	double relTriggerTime = _trigger - dataTimeWindow().startTime();

	double dt1 = relTriggerTime + _config.signalBegin;
	double dt2 = relTriggerTime + _config.signalEnd;
	double snr = -1;

	// Calculate data array indicies of requested time window
	int i1 = int(dt1*_fsamp);
	int i2 = int(dt2*_fsamp);

	// Calculate the initial trigger time
	int triggerIdx = relTriggerTime*_fsamp;
	int lowerUncertainty = -1;
	int upperUncertainty = -1;

	if ( !calculatePick(continuousData().size(), continuousData().typedData(),
	                    i1, i2, triggerIdx, lowerUncertainty, upperUncertainty, snr) ) {
		setStatus(Error, 0.0);
		return;
	}

	Core::Time pickTime = dataTimeWindow().startTime() + Core::TimeSpan(triggerIdx/_fsamp);

	// Debug: print the time difference between the pick and the initial trigger
	SEISCOMP_DEBUG("Picker::process repick result: dt=%.3f  snr=%.2f",
	               (double)(pickTime - _trigger), snr);

	if ( snr >= _config.snrMin ) {
		setStatus(Finished, 100.);
		Result res;
		res.record = record;
		res.snr = snr;
		res.time = pickTime;
		res.timeLowerUncertainty = (double)lowerUncertainty / _fsamp;
		res.timeUpperUncertainty = (double)upperUncertainty / _fsamp;
		res.timeWindowBegin = (double)(timeWindow().startTime() - pickTime);
		res.timeWindowEnd = (double)(timeWindow().endTime() - pickTime);
		emitPick(res);
	}
	else
		setStatus(LowSNR, snr);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Picker::writeData() {
	if ( !_lastRecord ) return;

	std::ofstream of((_lastRecord->streamID() + "-postpick.data").c_str());

	of << "#sampleRate: " << _lastRecord->samplingFrequency() << std::endl;

	for ( int i = 0; i < continuousData().size(); ++i )
		of << i << "\t" << continuousData()[i] << std::endl;
	of.close();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}

}
