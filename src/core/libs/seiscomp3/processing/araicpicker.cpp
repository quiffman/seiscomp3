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

#define SEISCOMP_COMPONENT AICPicker

#include <seiscomp3/logging/log.h>
#include <seiscomp3/processing/araicpicker.h>


namespace Seiscomp {

namespace Processing {

REGISTER_POSTPICKPROCESSOR(ARAICPicker, "AIC");

namespace {


//
// AIC repicker using the simple non-AR algorithm of Maeda (1985),
// see paper of Zhang et al. (2003) in BSSA
//

template<typename TYPE>
static void
maeda_aic(int n, TYPE *data, int &kmin, double &snr, int margin=10)
{
	// expects a properly filtered and demeaned trace

	// square trace in place
	for ( int i=0; i<n; ++i ) {
		data[i] = data[i]*data[i];
	}

	// windowed sum for variance computation
	double sumwin1 = 0, sumwin2 = 0, minaic = 0;
	int imin = margin, imax = n-margin;
	for ( int i = 0; i < n; ++i ) {
		if ( i < imin )
			sumwin1 += data[i];
		else
			sumwin2 += data[i];
	}
	
	for ( int k = imin; k < imax; ++k ) {
		double var1 = sumwin1/(k-1),
		       var2 = sumwin2/(n-k-1);
		double aic = k*log10(var1) + (n-k-1)*log10(var2);

		sumwin1 += data[k];
		sumwin2 -= data[k];

		data[k] = aic;
		if ( k == imin ) minaic = aic;
		if ( aic < minaic ) {
			minaic = aic;
			kmin = k;
			snr = var2/var1;
		}
	}

	double maxaic = data[imin] > data[imax-1] ? data[imin] : data[imax-1];

	for ( int k = 0; k < imin; ++k ) {
		data[k] = data[n-k-1] = maxaic;
	}
}


template<typename TYPE>
static void
maeda_aic_const(int n, const TYPE *data, int &kmin, double &snr, int margin=10)
{
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


template<typename TYPE>
static bool repick(int n, const TYPE *data, int &kmin, double &snr)
{
	if (n<=0) return false;

	double average = 0;
	int n2 = n/2; // use only first half of seismogram
	// FIXME somewhat hackish but better than nothing
	for (int i=0; i<n2; i++)
		average += data[i];
	average /= n2;

	std::vector<TYPE> tmp(n);
	for (int i=0; i<n; i++) {
		tmp[i] = data[i]-average;

	}
	kmin = -1;
	snr = -1;
	maeda_aic_const(n, &tmp[0], kmin, snr);

	return true;
}


}


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ARAICPicker::ARAICPicker() {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ARAICPicker::ARAICPicker(const Core::Time& trigger)
 : Picker(trigger) {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ARAICPicker::~ARAICPicker() {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool ARAICPicker::setup(const Settings &settings) {
	if ( !Picker::setup(settings) ) return false;

	settings.getValue(_config.signalBegin, "picker.AIC.signalBegin");
	settings.getValue(_config.signalEnd, "picker.AIC.signalEnd");
	settings.getValue(_config.snrMin, "picker.AIC.minSNR");

	std::string filter;
	settings.getValue(filter, "picker.AIC.filter");
	if ( !filter.empty() ) {
		std::string error;
		Filter *f = Filter::Create(filter, &error);
		if ( f == NULL ) {
			SEISCOMP_ERROR("failed to create filter '%s': %s",
			               filter.c_str(), error.c_str());
			return false;
		}
		setFilter(f);
	}

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
const std::string &ARAICPicker::methodID() const {
	static string method = "AIC";
	return method;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool ARAICPicker::calculatePick(int n, const double *data,
                                int signalStartIdx, int signalEndIdx,
                                int &triggerIdx, int &lowerUncertainty,
                                int &upperUncertainty, double &snr) {
	int relTriggerIdx = triggerIdx - signalStartIdx;
	if ( !repick(signalEndIdx-signalStartIdx, data+signalStartIdx, relTriggerIdx, snr) )
		return false;

	triggerIdx = relTriggerIdx + signalStartIdx;
	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}

}
