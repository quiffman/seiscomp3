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



#include <seiscomp3/math/conversions.h>


namespace Seiscomp {
namespace Math {


void np2deg(NODAL_PLANE &np) {
	np.dip = rad2deg(np.dip);
	np.str = rad2deg(np.str);
	np.rake = rad2deg(np.rake);
}


}
}
