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




#ifndef __SEISCOMP_LOCATOR_H__
#define __SEISCOMP_LOCATOR_H__


//#include <seiscomp3/gui/core/api.h>
#include <seiscomp3/seismology/locsat.h>


namespace Seiscomp {


class Locator : public Seiscomp::LocSAT {
	public:
		DataModel::Origin* relocate(DataModel::Origin* origin);
};


}



#endif
