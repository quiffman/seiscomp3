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



#define SEISCOMP_COMPONENT AmplitudeMLv

#include <seiscomp3/processing/amplitudeprocessor_MLv.h>
#include <seiscomp3/math/mean.h>
#include <seiscomp3/math/filter/seismometers.h>
#include <seiscomp3/math/deconvolution/fft.h>


using namespace Seiscomp::Math;

namespace Seiscomp {

namespace Processing {


IMPLEMENT_SC_CLASS_DERIVED(AmplitudeProcessor_MLv, AmplitudeProcessor, "AmplitudeProcessor_MLv");
REGISTER_AMPLITUDEPROCESSOR(AmplitudeProcessor_MLv, "MLv");
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
AmplitudeProcessor_MLv::AmplitudeProcessor_MLv()
	: AmplitudeProcessor("MLv") {
	setSignalEnd(150.);
	setMinSNR(0);
	setMaxDist(8);
	_computeAbsMax = true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
AmplitudeProcessor_MLv::AmplitudeProcessor_MLv(const Core::Time& trigger)
	: AmplitudeProcessor(trigger, "MLv") {
	setSignalEnd(150.);
	setMinSNR(0);
	setMaxDist(8);
	computeTimeWindow();
	_computeAbsMax = true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void AmplitudeProcessor_MLv::initFilter(double fsamp) {
	if ( !_enableResponses ) {
		AmplitudeProcessor::setFilter(
			new Filtering::IIR::WoodAndersonFilter<double>(Velocity)
		);
	}
	else
		AmplitudeProcessor::setFilter(NULL);

	AmplitudeProcessor::initFilter(fsamp);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int AmplitudeProcessor_MLv::capabilities() const {
	return MeasureType;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
AmplitudeProcessor::IDList
AmplitudeProcessor_MLv::capabilityParameters(Capability cap) const {
	if ( cap == MeasureType ) {
		IDList params;
		params.push_back("AbsMax");
		params.push_back("MinMax");
		return params;
	}

	return AmplitudeProcessor::capabilityParameters(cap);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool AmplitudeProcessor_MLv::setParameter(Capability cap, const std::string &value) {
	if ( cap == MeasureType ) {
		if ( value == "AbsMax" ) {
			_computeAbsMax = true;
			return true;
		}
		else if ( value == "MinMax" ) {
			_computeAbsMax = false;
			return true;
		}

		return false;
	}

	return AmplitudeProcessor::setParameter(cap, value);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool AmplitudeProcessor_MLv::deconvolveData(Response *resp,
                                            DoubleArray &data,
                                            int numberOfIntegrations) {
	Math::Deconvolution::FFT::TransferFunctionPtr tf =
		resp->getTransferFunction(numberOfIntegrations);

	if ( tf == NULL ) {
		setStatus(DeconvolutionFailed, 0);
		return false;
	}

	Math::Deconvolution::FFT::PolesAndZeros woodAnderson(
		Math::SeismometerResponse::WoodAnderson(Math::Velocity)
	);

	Math::Deconvolution::FFT::TransferFunctionPtr cascade =
		*tf / woodAnderson;

	// Remove linear trend
	double m,n;
	Math::Statistics::computeLinearTrend(data.size(), data.typedData(), m, n);
	Math::Statistics::detrend(data.size(), data.typedData(), m, n);

	return Math::Deconvolution::transformFFT(data.size(), data.typedData(),
	                                         _fsamp, cascade.get(),
	                                         _config.respTaper, _config.respMinFreq, _config.respMaxFreq);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool AmplitudeProcessor_MLv::computeAmplitude(
		const DoubleArray &data,
		size_t i1, size_t i2,
		size_t si1, size_t si2,
		double offset, double *dt,
		double *amplitude, double *width, double *period, double *snr)
{
	double amax;

	if ( _computeAbsMax ) {
		size_t imax = find_absmax(data.size(), data.typedData(), si1, si2, offset);
		amax = fabs(data[imax] - offset);
		*dt = imax;
	}
	else {
		int lmin, lmax;
		find_minmax(lmin, lmax, data.size(), data.typedData(), si1, si2, offset);
		amax = data[lmax] - data[lmin];
		*dt = (lmin+lmax)*0.5;
		*width = fabs(*dt-(double)lmin);
	}

	if ( *_noiseAmplitude == 0. )
		*snr = 1000000.0;
	else
		*snr = amax / *_noiseAmplitude;

	if ( *snr < _config.snrMin ) {
		setStatus(LowSNR, *snr);
		return false;
	}

	*period = -1;

	*amplitude = amax;

	if ( _streamConfig[_usedComponent].gain != 0.0 )
		*amplitude /= _streamConfig[_usedComponent].gain;
	else {
		setStatus(MissingGain, 0.0);
		return false;
	}

	// - convert to millimeter
	*amplitude *= 1E03;

	// - estimate peak-to-peak from absmax amplitude
	if ( _computeAbsMax )
		*amplitude *= 2.0;

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
double AmplitudeProcessor_MLv::timeWindowLength(double distance_deg) const {
	// Minimal S/SW group velocity.
	//
	// This is very approximate and may need refinement. Usually the Lg
	// group velocity is around 3.2-3.6 km/s. By setting v_min to 3 km/s,
	// we are probably on the safe side. We add 30 s to coount for rupture
	// duration, which may, however, nit be sufficient.
	double v_min = 3;

	double distance_km = distance_deg*111.2;
	double windowLength = distance_km/v_min + 30; 
	return windowLength;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}

}
