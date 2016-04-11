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


#ifndef _SEISCOMP_WAVEFORMPROCESSOR_H_
#define _SEISCOMP_WAVEFORMPROCESSOR_H_


#include <seiscomp3/core/record.h>
#include <seiscomp3/core/recordsequence.h>
#include <seiscomp3/core/typedarray.h>
#include <seiscomp3/core/enumeration.h>
#include <seiscomp3/math/filter.h>
#include <seiscomp3/processing/processor.h>
#include <seiscomp3/processing/stream.h>


namespace Seiscomp {
namespace Processing {


DEFINE_SMARTPOINTER(WaveformProcessor);


class SC_CORE_PROCESSING_API WaveformProcessor : public Processor {
	DECLARE_SC_CLASS(WaveformProcessor);

	// ----------------------------------------------------------------------
	//  Public types
	// ----------------------------------------------------------------------
	public:
		enum Component {
			VerticalComponent         = 0,  /* usually Z */
			FirstHorizontalComponent  = 1,  /* usually N */
			SecondHorizontalComponent = 2   /* usually E */
		};

		enum StreamComponent {
			Vertical            = 0,  /* usually Z */
			FirstHorizontal     = 1,  /* usually N */
			SecondHorizontal    = 2,  /* usually E */
			Horizontal,               /* usually N + E */
			Any                       /* all available components */
		};

		typedef Math::Filtering::InPlaceFilter<double> Filter;

		enum ProcessingHint {
			Distance,
			Depth
		};

		MAKEENUM(
			SignalUnit,
			EVALUES(
				//! Displacement
				Meter,
				//! Velocity
				MeterPerSecond,
				//! Acceleration
				MeterPerSecondSquared
			),
			// SEED names: see SEEDManual, ch.5 / 34, p.47
			ENAMES(
				"M",
				"M/S",
				"M/S**2"
			)
		);


		MAKEENUM(
			Status,
			EVALUES(
				//! No associated value
				WaitingForData,
				//! Associated value is progress in [0,100]
				InProgress,
				//! Associated value is 100
				Finished,
				//! Associated value is the last status
				Terminated,
				//! Associated value is the failed snr value
				LowSNR,
				//! No associated value yet
				QCError,
				//! Data is clipped
				DataClipped,
				//! Error during deconvolution
				DeconvolutionFailed,
				//! Distance hint is out of range to continue processing
				DistanceOutOfRange,
				//! Depth hint is out of range to continue processing
				DepthOutOfRange,
				//! Unit is not supported, e.g. m/s <> m/s**2
				IncompatibleUnit,
				//! Orientation missing
				MissingOrientation,
				//! Gain missing
				MissingGain,
				//! Response missing
				MissingResponse,
				//! No associated value yet (error code?)
				Error
			),
			ENAMES(
				"waiting for data",
				"in progress",
				"finished",
				"terminated",
				"low SNR",
				"QC error",
				"data clipped",
				"deconvolution failed",
				"distance out of range",
				"depth out of range",
				"incompatible unit",
				"missing orientation",
				"missing gain",
				"missing response",
				"error"
			)
		);


	// ----------------------------------------------------------------------
	//  X'truction
	// ----------------------------------------------------------------------
	public:
		//! C'tor
		WaveformProcessor(const Core::TimeSpan &initTime=0.0,
		                  const Core::TimeSpan &gapThreshold=0.1);

		//! D'tor
		virtual ~WaveformProcessor();


	// ----------------------------------------------------------------------
	//  Public interface
	// ----------------------------------------------------------------------
	public:
		//! Call this method in derived classes to make sure
		//! the filters samplingFrequency will be set correctly
		//! The feed method filters the data according to the
		//! set filter and calls process
		virtual bool feed(const Record *record);

		//! Convenience function to feed a whole record sequence into
		//! a WaveformProcessor.
		//! feed(const Record *) is used to feed a single record
		//! @return The number of records successfully fed
		int feedSequence(const RecordSequence *sequence);

		//! Sets a userdata pointer that is managed by the processor
		//! (stored inside a SmartPointer).
		void setUserData(Core::BaseObject *obj) const;

		//! Returns the set userdata object. This object must not be
		//! deleted by the caller. It is managed by the processor and
		//! dereferenced (and deleted) automatically. But the
		//! caller may hold a SmartPointer to that object.
		Core::BaseObject *userData() const;

		//! Initialize the filter for the given sampling frequency
		virtual void initFilter(double fsamp);

		//! Sets the filter to apply 
		virtual void setFilter(Filter *filter);

		//! The gain is input unit -> sensor unit
		Stream &streamConfig(Component c);
		const Stream &streamConfig(Component c) const;

		//! Sets the component to use to calculate the amplitude for
		void setUsedComponent(StreamComponent c) { _usedComponent = c; }

		//! Returns the used component
		StreamComponent usedComponent() const { return _usedComponent; }

		//! Sets the maximal gap length in seconds for that
		//! missing samples are handled or tolerated. Default: no tolerance
		void setGapTolerance(const Core::TimeSpan &length);
		const Core::TimeSpan &gapTolerance() const;

		//! Enables/disables the linear interpolation of missing samples
		//! inside a set gap tolerance
		void setGapInterpolationEnabled(bool enable);
		bool isGapInterpolationEnabled() const;

		//! Resets the processor completely. The configured init time
		//! is going to be processed again.
		virtual void reset();

		//! Returns the data's sampling frequency
		double samplingFrequency() const;

		//! This methods can be called to set a hint for the
		//! processor even while processing.
		//! The hint can be used to tune certain processing
		//! steps. Currently exists a distance and a depth hint.
		//! Derived classes should reimplement this method to react
		//! on this hints.
		//! E.g. an amplitude processor can cut its endtime when
		//! receiving a distance hint.
		//! The default implementation does nothing.
		virtual void setHint(ProcessingHint, double) {}

		void setEnabled(bool e);
		bool isEnabled() const { return _enabled; }

		//! Returns the timewindow of already processed data
		Core::TimeWindow dataTimeWindow() const;

		//! Returns the last fed record
		const Record *lastRecord() const;

		//! Returns the current status of the processor
		Status status() const;

		//! Returns the value associated with the status
		double statusValue() const;

		//! Terminates the processor ignoring its current state
		void terminate();

		//! Default implementation returns if the status if greater
		//! than InProgress.
		virtual bool isFinished() const;

		//! Closes the processor meaning that no more records
		//! are going to be fed in. The processing has been finished.
		virtual void close() const;


	// ----------------------------------------------------------------------
	//  Protected interface
	// ----------------------------------------------------------------------
	protected:
		//! Callback methods to react on changes of the
		//! enable state
		virtual void disabled() {}
		virtual void enabled() {}

		//! Virtual method that must be used in derived classes to analyse a datastream.
		//! It gives the raw record and the filtered data array as parameter.
		virtual void process(const Record *record, const DoubleArray &filteredData) = 0;

		//! Handles gaps. Returns whether the gap has been handled or not.
		virtual bool handleGap(Filter *filter, const Core::TimeSpan&, double lastSample, double nextSample, size_t missingSamples);

		virtual void fill(size_t n, double *samples);

		void setStatus(Status status, double value);


	// ----------------------------------------------------------------------
	//  Members
	// ----------------------------------------------------------------------
	protected:
		Filter          *_filter;

		RecordCPtr       _lastRecord;
		bool             _enabled;

		double           _fsamp;

		Core::TimeSpan   _initTime;

		Core::TimeWindow _dataTimeWindow;

		Core::Time       _lastSampleTime;
		double           _lastSample;

		Core::TimeSpan   _gapThreshold; /* threshold to recognize a gap */
		Core::TimeSpan   _gapTolerance; /* gap length to tolerate */

		bool             _enableGapInterpolation; /* default: false */

		size_t           _receivedSamples;
		size_t           _neededSamples;

		bool             _initialized;

		StreamComponent  _usedComponent; /* default: vertical */
		Stream           _streamConfig[3];


	private:
		Status           _status;
		double           _statusValue;

		mutable Core::BaseObjectPtr _userData;
};


}
}


#endif
