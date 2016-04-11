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


#ifndef __SEISCOMP_PROCESSING_PICKER_S_L2_H__
#define __SEISCOMP_PROCESSING_PICKER_S_L2_H__


#include <seiscomp3/processing/secondarypicker.h>


namespace Seiscomp {
namespace Processing {


class SC_CORE_PROCESSING_API SL2Picker : public SecondaryPicker {
	// ----------------------------------------------------------------------
	//  X'truction
	// ----------------------------------------------------------------------
	public:
		//! C'tor
		SL2Picker();

		//! D'tor
		~SL2Picker();


	// ----------------------------------------------------------------------
	//  Public Interface
	// ----------------------------------------------------------------------
	public:
		bool setup(const Settings &settings);

		const std::string &methodID() const;


	protected:
		void fill(size_t n, double *samples);
		void process(const Record *rec, const DoubleArray &filteredData);

	private:
		bool    _initialized;
		double  _threshold;
		double  _minSNR;
		double  _margin;
		double  _timeCorr;
		Result  _result;
		Filter *_compFilter;
};


}
}

#endif
