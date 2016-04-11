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


#define SEISCOMP_COMPONENT CutOff

#include <math.h>

#include <seiscomp3/math/filter/cutoff.h>
#include <seiscomp3/core/exceptions.h>
#include<seiscomp3/logging/log.h>


namespace Seiscomp
{
namespace Math
{
namespace Filtering
{

namespace _private {

class FilterException : public Seiscomp::Core::GeneralException {
	public:
		FilterException() : GeneralException("filter exception") {}
		FilterException(std::string what) : GeneralException(what) {}
};


}

template<typename TYPE>
CutOff<TYPE>::CutOff(double threshold)
 : _threshold(threshold) {}


template<typename TYPE>
void CutOff<TYPE>::apply(int n, TYPE *inout) {
	if ( _threshold <= 0.0 )
		throw _private::FilterException("Threshold not initialized");
	if ( n < 2 ) return;

	for ( int i = 0, j = 1; j < n; ++i, ++j ) {
		double diff = fabs(inout[i] - inout[j]);
		if ( diff >= _threshold ) {
			int tmp = (i > 0) ? i - 1 : 0;
			inout[j] = (inout[i] + inout[tmp]) / 2;
		}
	}
}

template<typename TYPE>
void CutOff<TYPE>::setSamplingFrequency(double fsamp) {}

template<typename TYPE>
InPlaceFilter<TYPE>* CutOff<TYPE>::clone() const {
	return new CutOff<TYPE>(_threshold);
}

template<typename TYPE>
int CutOff<TYPE>::setParameters(int n, const double *params) {
	if ( n != 1 ) return 1;
	if ( params[0] <= 0 )
		return -1;

	_threshold = params[0];
	return n;
}


INSTANTIATE_INPLACE_FILTER(CutOff, SC_CORE_MATH_API);
REGISTER_INPLACE_FILTER(CutOff, "CUTOFF");


} // namespace Seiscomp::Math::Filtering

} // namespace Seiscomp::Math

} // namespace Seiscomp
