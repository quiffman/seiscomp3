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


#define SEISCOMP_COMPONENT MLv

#include <seiscomp3/logging/log.h>
#include <seiscomp3/processing/magnitudeprocessor_MLv.h>
#include <seiscomp3/seismology/magnitudes.h>


namespace Seiscomp {

namespace Processing {


IMPLEMENT_SC_CLASS_DERIVED(MagnitudeProcessor_MLv, MagnitudeProcessor, "MagnitudeProcessor_MLv");
REGISTER_MAGNITUDEPROCESSOR(MagnitudeProcessor_MLv, "MLv");
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
MagnitudeProcessor_MLv::MagnitudeProcessor_MLv()
 : MagnitudeProcessor("MLv") {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
MagnitudeProcessor::Status MagnitudeProcessor_MLv::computeMagnitude(
	double amplitude, // in micrometers per second
	double,           // period is unused
	double delta,     // in degrees
	double depth,     // in kilometers
	double &value)
{
	// Clip depth to 0
	if ( depth < 0 ) depth = 0;

	if ( !Magnitudes::compute_ML(amplitude, delta, depth, &value) )
		return Error;

	value = correctMagnitude(value);

	return OK;
}


}

}
