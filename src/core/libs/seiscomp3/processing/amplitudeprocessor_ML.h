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



#ifndef __SEISCOMP_PROCESSING_AMPLITUDEPROCESSOR_ML_H__
#define __SEISCOMP_PROCESSING_AMPLITUDEPROCESSOR_ML_H__

#include <seiscomp3/processing/amplitudeprocessor_MLv.h>
#include <seiscomp3/processing/api.h>


namespace Seiscomp {

namespace Processing {


//! Wrapper class that allows access to protected methods for
//! the proxy (see below).
class AmplitudeProcessor_MLh : public AmplitudeProcessor_MLv {
	public:
		AmplitudeProcessor_MLh();

	friend class AmplitudeProcessor_ML;
};


//! Proxy amplitude processor that holds to MLv processors to calculate
//! the amplitudes on both horizontals and then averages the result.
//! This class does not handle waveforms itself. It directs them to the
//! corresponding amplitude processors instead.
class SC_CORE_PROCESSING_API AmplitudeProcessor_ML : public AmplitudeProcessor {
	DECLARE_SC_CLASS(AmplitudeProcessor_ML);

	public:
		AmplitudeProcessor_ML();
		AmplitudeProcessor_ML(const Core::Time& trigger);

	public:
		int capabilities() const;
		IDList capabilityParameters(Capability cap) const;
		bool setParameter(Capability cap, const std::string &value);

		bool setup(const Settings &settings);

		void setTrigger(const Core::Time& trigger) throw(Core::ValueException);

		void computeTimeWindow();

		void reset();
		void close();

		bool feed(const Record *record);

		bool computeAmplitude(const DoubleArray &data,
		                      size_t i1, size_t i2,
		                      size_t si1, size_t si2,
		                      double offset, double *dt,
		                      double *amplitude, double *width,
		                      double *period, double *snr);

		const AmplitudeProcessor *componentProcessor(Component comp) const;
		const DoubleArray *processedData(Component comp) const;

		void reprocess(OPT(double) searchBegin, OPT(double) searchEnd);


	protected:
		double timeWindowLength(double distance) const;


	private:
		void newAmplitude(const AmplitudeProcessor *proc,
		                  const AmplitudeProcessor::Result &res);


	private:
		enum CombinerProc {
			TakeMin,
			TakeMax,
			TakeAverage
		};

		mutable
		AmplitudeProcessor_MLh _ampE, _ampN;
		OPT(double)            _amplitudes[2];
		double                 _amplitudeWidths[2];
		Core::Time             _times[2];
		double                 _timeWindowBegin;
		double                 _timeWindowEnd;
		CombinerProc           _combiner;
};


}

}


#endif
