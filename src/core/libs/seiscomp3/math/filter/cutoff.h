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


#ifndef _SEISCOMP_MATH_FILTER_CUTOFF_H_
#define _SEISCOMP_MATH_FILTER_CUTOFF_H_

#include<vector>
#include<seiscomp3/math/filter.h>


namespace Seiscomp
{
namespace Math
{
namespace Filtering
{

template<typename TYPE>
class CutOff : public InPlaceFilter<TYPE> {
	public:
		CutOff(double threshold = 0.0);

	public:
		virtual void setSamplingFrequency(double fsamp);
		virtual int setParameters(int n, const double *params);

		// apply filter to data vector **in*place**
		virtual void apply(int n, TYPE *inout);
		virtual InPlaceFilter<TYPE>* clone() const;

	private:
		double _threshold;
};


} // namespace Seiscomp::Math::Filtering

} // namespace Seiscomp::Math

} // namespace Seiscomp

#endif

