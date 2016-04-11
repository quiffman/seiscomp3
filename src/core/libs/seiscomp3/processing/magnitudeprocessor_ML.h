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



#ifndef __SEISCOMP_PROCESSING_MAGNITUDEPROCESSOR_ML_H__
#define __SEISCOMP_PROCESSING_MAGNITUDEPROCESSOR_ML_H__

#include <seiscomp3/processing/magnitudeprocessor.h>
#include <seiscomp3/processing/api.h>


namespace Seiscomp {

namespace Processing {


class SC_CORE_PROCESSING_API MagnitudeProcessor_ML : public MagnitudeProcessor {
	DECLARE_SC_CLASS(MagnitudeProcessor_ML);

	public:
		MagnitudeProcessor_ML();


	public:
		std::string amplitudeType() const;

		Status computeMagnitude(
			double amplitude, // in micrometers per second
			double period,      // in seconds
			double delta,     // in degrees
			double depth,     // in kilometers
			double &value);
};


}

}


#endif
