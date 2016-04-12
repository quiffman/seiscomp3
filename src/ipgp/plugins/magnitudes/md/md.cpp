/************************************************************************
 *                                                                      *
 * Copyright (C) 2012 OVSM/IPGP                                         *
 *                                                                      *
 * This program is free software: you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * This program is part of 'Projet TSUAREG - INTERREG IV Caraïbes'.     *
 * It has been co-financed by the European Union and le Minitère de     *
 * l'Ecologie, du Développement Durable, des Transports et du Logement. *
 *                                                                      *
 ************************************************************************/


#define SEISCOMP_COMPONENT Md

#include "md.h"
#include <seiscomp3/processing/waveformprocessor.h>
#include <seiscomp3/math/filter/stalta.h>
#include <seiscomp3/math/filter/iirfilter.h>
#include <seiscomp3/math/filter/butterworth.h>
#include <seiscomp3/logging/log.h>
#include <seiscomp3/core/strings.h>
#include <seiscomp3/seismology/magnitudes.h>
#include <seiscomp3/math/mean.h>
#include <seiscomp3/math/filter/seismometers.h>
#include <seiscomp3/math/restitution/fft.h>
#include <seiscomp3/math/geo.h>
#include <iostream>
#include <boost/bind.hpp>
#include <unistd.h>


#define _DEPTH_MAX 200.0
#define _SIGNAL_WINDOW_END 30.0
#define _LINEAR_CORRECTION 1.0
#define _OFFSET 0.0
#define _SNR_MIN 1.2
#define _DELTA_MAX 400.0
#define _MD_MAX 5.0
#define _FMA -0.87
#define _FMB 2.0
#define _FMD 0.0035
#define _FMF 0.0
#define _FMZ 0.0
#define _STACOR 0.0 //! not fully implemented !
/**
 * Seismometer selection
 * 1 for Wood-Anderson
 * 2 for Seismometer 5 sec
 * 3 for WWSSN LP ? filter
 * 4 for WWSSN SP? filter
 * 5 for Generic Seismometer ? filter
 * 6 for Butterworth Low Pass ? filter
 * 7 for Butterwoth High Pass ? filter
 * 8 for Butterworth Band Pass ? filter
 **/
#define _SEISMO 1
#define _BUTTERWORTH "3,1,15"


ADD_SC_PLUGIN("Md duration magnitude plugin", "IPGP <www.ipgp.fr>", 0, 1, 0)

using namespace Seiscomp;
using namespace Seiscomp::Math;
using namespace Seiscomp::Processing;


/*----[ AMPLITUDE PROCESSOR CLASS ]----*/

IMPLEMENT_SC_CLASS_DERIVED(AmplitudeProcessor_Md, AmplitudeProcessor, "AmplitudeProcessor_Md");
REGISTER_AMPLITUDEPROCESSOR(AmplitudeProcessor_Md, "Md");

struct ampConfig {

		double DEPTH_MAX;
		double SIGNAL_WINDOW_END;
		double SNR_MIN;
		double DELTA_MAX;
		double MD_MAX;
		double FMA;
		double FMB;
		double FMD;
		double FMF;
		double FMZ;
		double STACOR;
		int SEISMO;
		std::string BUTTERWORTH;
};
ampConfig aFile;



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
AmplitudeProcessor_Md::AmplitudeProcessor_Md() :
		AmplitudeProcessor("Md") {

	setSignalStart(0.);
	setSignalEnd(150.);
	setMinSNR(aFile.SNR_MIN);
	setMaxDist(8);
	_computeAbsMax = true;

	SEISCOMP_DEBUG("[plugin] [Md] Initialized using default constructor");
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
AmplitudeProcessor_Md::AmplitudeProcessor_Md(const Core::Time& trigger) :
		AmplitudeProcessor(trigger, "Md") {

	setSignalStart(0.);
	setSignalEnd(150.);
	setMinSNR(aFile.SNR_MIN);
	setMaxDist(8);
	_computeAbsMax = true;
	computeTimeWindow();

	SEISCOMP_DEBUG("[plugin] [Md] Initialized using Time constructor");
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool AmplitudeProcessor_Md::setup(const Settings& settings) {

	bool isButterworth = false;
	try {
		aFile.SEISMO = settings.getInt("md.seismo");
		std::string type;
		switch ( aFile.SEISMO ) {
			case 1:
				type = "WoodAnderson";
				break;
			case 2:
				type = "Seismo5sec";
				break;
			case 3:
				type = "WWSSN LP";
				break;
			case 4:
				type = "WWSSN SP";
				break;
			case 5:
				type = "Generic Seismometer";
				break;
			case 6:
				type = "Butterworth Low Pass";
				isButterworth = true;
				break;
			case 7:
				type = "Butterworth High Pass";
				isButterworth = true;
				break;
			case 8:
				type = "Butterworth Band Pass";
				isButterworth = true;
				break;
			default:
				break;
		}
		SEISCOMP_DEBUG("[plugin] [Md] sets SEISMO to  %s [%s.%s]", type.c_str(),
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}
	catch ( ... ) {
		aFile.SEISMO = _SEISMO;
		SEISCOMP_ERROR("[plugin] [Md] can not read SEISMO value from configuration file [%s.%s]",
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}

	if ( isButterworth == true ) {
		try {
			aFile.BUTTERWORTH = settings.getString("md.butterworth");
			SEISCOMP_DEBUG("[plugin] [Md] sets Butterworth filter to  %s [%s.%s]",
			    aFile.BUTTERWORTH.c_str(), settings.networkCode.c_str(),
			    settings.stationCode.c_str());
		}
		catch ( ... ) {
			aFile.BUTTERWORTH = _BUTTERWORTH;
			SEISCOMP_ERROR("[plugin] [Md] can not read Butterworth filter value from configuration file [%s.%s]",
			    settings.networkCode.c_str(), settings.stationCode.c_str());
		}
	}

	try {
		aFile.DEPTH_MAX = settings.getDouble("md.depthmax");
		SEISCOMP_DEBUG("[plugin] [Md] sets DEPTH MAX to  %.2f [%s.%s]", aFile.DEPTH_MAX,
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}
	catch ( ... ) {
		aFile.DEPTH_MAX = _DEPTH_MAX;
		SEISCOMP_ERROR("[plugin] [Md] can not read DEPTH MAX value from configuration file [%s.%s]",
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}

	try {
		aFile.DELTA_MAX = settings.getDouble("md.deltamax");
		SEISCOMP_DEBUG("[plugin] [Md] sets DELTA MAX to  %.2f [%s.%s]", aFile.DELTA_MAX,
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}
	catch ( ... ) {
		aFile.DELTA_MAX = _DELTA_MAX;
		SEISCOMP_ERROR("[plugin] [Md] can not read DELTA MAX value from configuration file [%s.%s]",
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}

	try {
		aFile.SNR_MIN = settings.getDouble("md.snrmin");
		SEISCOMP_DEBUG("[plugin] [Md] sets SNR MIN to  %.2f [%s.%s]", aFile.SNR_MIN,
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}
	catch ( ... ) {
		aFile.SNR_MIN = _SNR_MIN;
		SEISCOMP_ERROR("[plugin] [Md] can not read SNR MIN value from configuration file [%s.%s]",
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}

	try {
		aFile.MD_MAX = settings.getDouble("md.mdmax");
		SEISCOMP_DEBUG("[plugin] [Md] sets MD MAX to  %.2f [%s.%s]", aFile.MD_MAX,
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}
	catch ( ... ) {
		aFile.MD_MAX = _MD_MAX;
		SEISCOMP_ERROR("[plugin] [Md] can not read MD MAX value from configuration file [%s.%s]",
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}

	try {
		aFile.FMA = settings.getDouble("md.fma");
		SEISCOMP_DEBUG("[plugin] [Md] sets FMA to  %.4f [%s.%s]", aFile.FMA,
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}
	catch ( ... ) {
		aFile.FMA = _FMA;
		SEISCOMP_ERROR("[plugin] [Md] can not read FMA value from configuration file [%s.%s]",
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}

	try {
		aFile.FMB = settings.getDouble("md.fmb");
		SEISCOMP_DEBUG("[plugin] [Md] sets FMB to  %.4f [%s.%s]", aFile.FMB,
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}
	catch ( ... ) {
		aFile.FMB = _FMB;
		SEISCOMP_ERROR("[plugin] [Md] can not read FMB value from configuration file [%s.%s]",
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}

	try {
		aFile.FMD = settings.getDouble("md.fmd");
		SEISCOMP_DEBUG("[plugin] [Md] sets FMD to  %.4f [%s.%s]", aFile.FMD,
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}
	catch ( ... ) {
		aFile.FMD = _FMD;
		SEISCOMP_ERROR("[plugin] [Md] can not read FMD value from configuration file [%s.%s]",
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}

	try {
		aFile.FMF = settings.getDouble("md.fmf");
		SEISCOMP_DEBUG("[plugin] [Md] sets FMF to  %.4f [%s.%s]", aFile.FMF,
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}
	catch ( ... ) {
		aFile.FMF = _FMF;
		SEISCOMP_ERROR("[plugin] [Md] can not read FMF value from configuration file [%s.%s]",
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}

	try {
		aFile.FMZ = settings.getDouble("md.fmz");
		SEISCOMP_DEBUG("[plugin] [Md] sets FMZ to  %.4f [%s.%s]", aFile.FMZ,
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}
	catch ( ... ) {
		aFile.FMZ = _FMZ;
		SEISCOMP_ERROR("[plugin] [Md] can not read FMZ value from configuration file [%s.%s]",
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}

	_isInitialized = true;

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void AmplitudeProcessor_Md::initFilter(double fsamp) {

	if ( !_enableResponses ) {

		Math::Filtering::InPlaceFilter<double>* f;
		switch ( aFile.SEISMO ) {
			case 1:
				AmplitudeProcessor::setFilter(new Filtering::IIR::WoodAndersonFilter<
				        double>(Velocity));
				break;
			case 2:
				AmplitudeProcessor::setFilter(new Filtering::IIR::Seismometer5secFilter<
				        double>(Velocity));
				break;
			case 3:
				AmplitudeProcessor::setFilter(new Filtering::IIR::WWSSN_LP_Filter<
				        double>(Velocity));
				break;
			case 4:
				AmplitudeProcessor::setFilter(new Filtering::IIR::WWSSN_SP_Filter<
				        double>(Velocity));
				break;
			case 5:
				AmplitudeProcessor::setFilter(new Filtering::IIR::GenericSeismometer<
				        double>(Velocity));
				break;
			case 6:
				f = new Math::Filtering::IIR::ButterworthLowpass<double>(3, 1, 15);
				AmplitudeProcessor::setFilter(f);
				break;
			case 7:
				f = new Math::Filtering::IIR::ButterworthHighpass<double>(3, 1, 15);
				AmplitudeProcessor::setFilter(f);
				break;
			case 8:
				f = new Math::Filtering::IIR::ButterworthBandpass<double>(3, 1, 15, 1, true);
				AmplitudeProcessor::setFilter(f);
				break;
			default:
				SEISCOMP_ERROR("[plugin] [Md] can not initialize the chosen filter, please review your configuration file");
				break;
		}
	}
	else
		AmplitudeProcessor::setFilter(NULL);

	AmplitudeProcessor::initFilter(fsamp);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int AmplitudeProcessor_Md::capabilities() const {
	return MeasureType;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
AmplitudeProcessor::IDList
AmplitudeProcessor_Md::capabilityParameters(Capability cap) const {

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
bool AmplitudeProcessor_Md::setParameter(Capability cap,
                                         const std::string& value) {

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
bool AmplitudeProcessor_Md::deconvolveData(Response* resp,
                                           DoubleArray& data,
                                           int numberOfIntegrations) {

	double m, n;
	Math::Restitution::FFT::TransferFunctionPtr tf = resp->getTransferFunction(numberOfIntegrations);
	if ( tf == NULL ) {
		setStatus(DeconvolutionFailed, 0);
		return false;
	}
	Math::Restitution::FFT::TransferFunctionPtr cascade;
	Math::Restitution::FFT::PolesAndZeros woodAnderson(Math::SeismometerResponse::WoodAnderson(Math::Velocity));
	Math::Restitution::FFT::PolesAndZeros seis5sec(Math::SeismometerResponse::Seismometer5sec(Math::Velocity));

	switch ( aFile.SEISMO ) {
		case 1:
			cascade = *tf / woodAnderson;
			break;
		case 2:
			cascade = *tf / seis5sec;
			break;
		default:
			SEISCOMP_INFO("[plugin] [Md] filter hasn't been set, applying Wood-Anderson");
			return false;
			break;
	}

	// Remove linear trend
	Math::Statistics::computeLinearTrend(data.size(), data.typedData(), m, n);
	Math::Statistics::detrend(data.size(), data.typedData(), m, n);

	return Math::Restitution::transformFFT(data.size(), data.typedData(),
	    _stream.fsamp, cascade.get(), _config.respTaper, _config.respMinFreq,
	    _config.respMaxFreq);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool AmplitudeProcessor_Md::computeAmplitude(const DoubleArray& data, size_t i1,
                                             size_t i2, size_t si1, size_t si2,
                                             double offset, AmplitudeIndex* dt,
                                             AmplitudeValue* amplitude,
                                             double* period, double* snr) {

	double amax, Imax, ofs_sig, amp_sig;
	DoubleArrayPtr d;

	if ( *snr < aFile.SNR_MIN )
		SEISCOMP_DEBUG("[plugin] [Md] computed snr is under configured SNR MIN");

	if ( _computeAbsMax ) {
		size_t imax = find_absmax(data.size(), data.typedData(), si1, si2, offset);
		amax = fabs(data[imax] - offset);
		dt->index = imax;
	}
	else {
		int lmin, lmax;
		find_minmax(lmin, lmax, data.size(), data.typedData(), si1, si2, offset);
		amax = data[lmax] - data[lmin];
		dt->index = (lmin + lmax) * 0.5;
		dt->begin = lmin - dt->index;
		dt->end = lmax - dt->index;
	}

	Imax = dt->index;

	SEISCOMP_DEBUG("[plugin] [Md] Amax: %.2f", amax);

	//! searching for Coda second by second through the end of the window
	//! TODO: elevate accuracy by using a nanometers scale (maybe)
	unsigned int i = si1;
	bool hasEndSignal = false;
	double calculatedSnr;

	for (i = (int) Imax; i < i2; i = i + 1 * (int) _stream.fsamp) {

		int window_end = i + 1 * (int) _stream.fsamp;
		d = static_cast<DoubleArray*>(data.slice(i, window_end));

		//! computes pre-arrival offset
		ofs_sig = d->median();

		//! computes rms after removing offset
		amp_sig = 2 * d->rms(ofs_sig);

		if ( amp_sig / *_noiseAmplitude <= _config.snrMin ) {
			SEISCOMP_DEBUG("[plugin] [Md] End of signal found! (%.2f <= %.2f)",
			    (amp_sig / *_noiseAmplitude), _config.snrMin);
			hasEndSignal = true;
			calculatedSnr = amp_sig / *_noiseAmplitude;
			break;
		}
	}

	if ( !hasEndSignal ) {
		SEISCOMP_ERROR("[plugin] [Md] SNR stayed over configured SNR_MIN! (%.2f > %.2f), skipping magnitude calculation for this station",
		    calculatedSnr, _config.snrMin);
		return false;
	}

	dt->index = i;

	//amplitude->value = 2 * amp_sig; //! actually it would have to be max. peak-to-peak
	amplitude->value = amp_sig;

	if ( _streamConfig[_usedComponent].gain != 0.0 )
		amplitude->value /= _streamConfig[_usedComponent].gain;
	else {
		setStatus(MissingGain, 0.0);
		return false;
	}

	// Convert m/s to nm/s
	amplitude->value *= 1.E09;


	*period = i - i1 + (_config.signalBegin * _stream.fsamp);

	SEISCOMP_DEBUG("[plugin] [Md] calculated event amplitude = %.2f", amplitude->value);
	SEISCOMP_DEBUG("[plugin] [Md] calculated signal end at %.2f ms from P phase", *period);

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
double AmplitudeProcessor_Md::timeWindowLength(double distance_deg) const {

	if ( !_isInitialized ) {

		aFile.MD_MAX = _MD_MAX;
		aFile.FMA = _FMA;
		aFile.FMZ = _FMZ;
		aFile.DEPTH_MAX = _DEPTH_MAX;
		aFile.STACOR = _STACOR;
		aFile.FMD = _FMD;
		aFile.FMB = _FMB;
		aFile.FMF = _FMF;
		aFile.SNR_MIN = _SNR_MIN;
		aFile.DELTA_MAX = _DELTA_MAX;
		aFile.SIGNAL_WINDOW_END = _SIGNAL_WINDOW_END;
		aFile.SEISMO = _SEISMO;
		aFile.BUTTERWORTH = _BUTTERWORTH;
	}

	double distance_km = Math::Geo::deg2km(distance_deg);
	double windowLength = (aFile.MD_MAX - aFile.FMA - (aFile.FMZ * aFile.DEPTH_MAX)
	        - aFile.STACOR - (aFile.FMD * distance_km)) / (aFile.FMB + aFile.FMF);

	windowLength = pow(10, windowLength) + aFile.SIGNAL_WINDOW_END;
	SEISCOMP_DEBUG("[plugin] [Md] Requesting stream of %.2fsec for current station", windowLength);

	return windowLength;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/*----[ END OF AMPLITUDE PROCESSOR CLASS ]----*/




/*----[ MAGNITUDE PROCESSOR CLASS ]----*/

IMPLEMENT_SC_CLASS_DERIVED(MagnitudeProcessor_Md, MagnitudeProcessor, "MagnitudeProcessor_Md");
REGISTER_MAGNITUDEPROCESSOR(MagnitudeProcessor_Md, "Md");

struct magConfig {

		double DEPTH_MAX;
		double LINEAR_CORRECTION;
		double OFFSET;
		double DELTA_MAX;
		double MD_MAX;
		double FMA;
		double FMB;
		double FMD;
		double FMF;
		double FMZ;
		double STACOR;
};
magConfig mFile;



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
MagnitudeProcessor_Md::MagnitudeProcessor_Md() :
		MagnitudeProcessor("Md") {

	_linearCorrection = mFile.LINEAR_CORRECTION;
	_constantCorrection = mFile.OFFSET;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool MagnitudeProcessor_Md::setup(const Settings& settings) {

	try {
		mFile.DELTA_MAX = settings.getDouble("md.deltamax");
		SEISCOMP_DEBUG("[plugin] [Md] sets DELTA MAX to  %.2f [%s.%s]", mFile.DELTA_MAX,
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}
	catch ( ... ) {
		mFile.DELTA_MAX = _DELTA_MAX;
		SEISCOMP_ERROR("[plugin] [Md] can not read DELTA MAX value from configuration file [%s.%s]",
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}

	try {
		mFile.DEPTH_MAX = settings.getDouble("md.depthmax");
		SEISCOMP_DEBUG("[plugin] [Md] sets DEPTH MAX to  %.2f [%s.%s]", mFile.DEPTH_MAX,
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}
	catch ( ... ) {
		mFile.DEPTH_MAX = _DEPTH_MAX;
		SEISCOMP_ERROR("[plugin] [Md] can not read DEPTH MAX value from configuration file [%s.%s]",
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}

	try {
		mFile.MD_MAX = settings.getDouble("md.mdmax");
		SEISCOMP_DEBUG("[plugin] [Md] sets MD MAX to  %.2f [%s.%s]", mFile.MD_MAX,
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}
	catch ( ... ) {
		mFile.MD_MAX = _MD_MAX;
		SEISCOMP_ERROR("[plugin] [Md] can not read MD MAX value from configuration file [%s.%s]",
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}

	try {
		mFile.LINEAR_CORRECTION = settings.getDouble("md.linearcorrection");
		SEISCOMP_DEBUG("[plugin] [Md] sets LINEAR CORRECTION to  %.2f [%s.%s]",
		    mFile.LINEAR_CORRECTION, settings.networkCode.c_str(), settings.stationCode.c_str());
	}
	catch ( ... ) {
		mFile.LINEAR_CORRECTION = _LINEAR_CORRECTION;
		SEISCOMP_ERROR("[plugin] [Md] can not read LINEAR CORRECTION value from configuration file [%s.%s]",
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}

	try {
		mFile.OFFSET = settings.getDouble("md.offset");
		SEISCOMP_DEBUG("[plugin] [Md] sets OFFSET to  %.2f [%s.%s]", mFile.OFFSET,
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}
	catch ( ... ) {
		mFile.OFFSET = _OFFSET;
		SEISCOMP_ERROR("[plugin] [Md] can not read OFFSET value from configuration file [%s.%s]",
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}

	try {
		mFile.FMA = settings.getDouble("md.fma");
		SEISCOMP_DEBUG("[plugin] [Md] sets FMA to  %.4f [%s.%s]", mFile.FMA,
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}
	catch ( ... ) {
		mFile.FMA = _FMA;
		SEISCOMP_ERROR("[plugin] [Md] can not read FMA value from configuration file [%s.%s]",
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}

	try {
		mFile.FMB = settings.getDouble("md.fmb");
		SEISCOMP_DEBUG("[plugin] [Md] sets FMB to  %.4f [%s.%s]", mFile.FMB,
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}
	catch ( ... ) {
		mFile.FMB = _FMB;
		SEISCOMP_ERROR("[plugin] [Md] can not read FMB value from configuration file [%s.%s]",
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}

	try {
		mFile.FMD = settings.getDouble("md.fmd");
		SEISCOMP_DEBUG("[plugin] [Md] sets FMD to  %.4f [%s.%s]", mFile.FMD,
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}
	catch ( ... ) {
		mFile.FMD = _FMD;
		SEISCOMP_ERROR("[plugin] [Md] can not read FMD value from configuration file [%s.%s]",
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}

	try {
		mFile.FMF = settings.getDouble("md.fmf");
		SEISCOMP_DEBUG("[plugin] [Md] sets FMF to  %.4f [%s.%s]", mFile.FMF,
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}
	catch ( ... ) {
		mFile.FMF = _FMF;
		SEISCOMP_ERROR("[plugin] [Md] can not read FMF value from configuration file [%s.%s]",
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}

	try {
		mFile.FMZ = settings.getDouble("md.fmz");
		SEISCOMP_DEBUG("[plugin] [Md] sets FMZ to  %.4f [%s.%s]", mFile.FMZ,
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}
	catch ( ... ) {
		mFile.FMZ = _FMZ;
		SEISCOMP_ERROR("[plugin] [Md] can not read FMZ value from configuration file [%s.%s]",
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}

	try {
		mFile.STACOR = settings.getDouble("md.stacor");
		SEISCOMP_DEBUG("[plugin] [Md] sets STACOR to  %.4f [%s.%s]", mFile.STACOR,
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}
	catch ( ... ) {
		mFile.STACOR = _STACOR;
		SEISCOMP_ERROR("[plugin] [Md] can not read STACOR value from configuration file [%s.%s]",
		    settings.networkCode.c_str(), settings.stationCode.c_str());
	}
	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
MagnitudeProcessor::Status
MagnitudeProcessor_Md::computeMagnitude(double amplitude, double period,
                                        double delta, double depth,
                                        double& value) {

	double epdistkm;
	epdistkm = Math::Geo::deg2km(delta);

	SEISCOMP_DEBUG("[plugin] [Md] --------------------------------");
	SEISCOMP_DEBUG("[plugin] [Md] |    PARAMETERS   |    VALUE   |");
	SEISCOMP_DEBUG("[plugin] [Md] --------------------------------");
	SEISCOMP_DEBUG("[plugin] [Md] | delta max       | %.2f ", mFile.DELTA_MAX);
	SEISCOMP_DEBUG("[plugin] [Md] | depth max       | %.2f ", mFile.DEPTH_MAX);
	SEISCOMP_DEBUG("[plugin] [Md] | md max          | %.2f ", mFile.MD_MAX);
	SEISCOMP_DEBUG("[plugin] [Md] | fma             | %.4f ", mFile.FMA);
	SEISCOMP_DEBUG("[plugin] [Md] | fmb             | %.4f ", mFile.FMB);
	SEISCOMP_DEBUG("[plugin] [Md] | fmd             | %.4f ", mFile.FMD);
	SEISCOMP_DEBUG("[plugin] [Md] | fmf             | %.4f ", mFile.FMF);
	SEISCOMP_DEBUG("[plugin] [Md] | fmz             | %.4f ", mFile.FMZ);
	SEISCOMP_DEBUG("[plugin] [Md] | stacor          | %.4f ", mFile.STACOR);
	SEISCOMP_DEBUG("[plugin] [Md] --------------------------------");
	SEISCOMP_DEBUG("[plugin] [Md] | (f-p)           | %.2f sec ", period);
	SEISCOMP_DEBUG("[plugin] [Md] | seismic depth   | %.2f km ", depth);
	SEISCOMP_DEBUG("[plugin] [Md] | epicenter dist  | %.2f km ", epdistkm);
	SEISCOMP_DEBUG("[plugin] [Md] --------------------------------");

	if ( amplitude <= 0. ) {
		value = 0;
		SEISCOMP_ERROR("[plugin] [Md] calculated amplitude is wrong, no magnitude will be calculated");
		return Error;
	}

	if ( (mFile.DELTA_MAX) < epdistkm ) {
		SEISCOMP_ERROR("[plugin] [Md] epicenter distance is out of configured range, no magnitude will be calculated");
		return DistanceOutOfRange;
	}

	value = mFile.FMA + mFile.FMB * log10(period) + (mFile.FMF * period)
	        + (mFile.FMD * epdistkm) + (mFile.FMZ * depth) + mFile.STACOR;

	if ( value > mFile.MD_MAX )
		SEISCOMP_WARNING("[plugin] [Md] calculated magnitude is beyond max Md value [value= %.2f]", value);
	return OK;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/*----[ END OF MAGNITUDE PROCESSOR CLASS ]----*/


