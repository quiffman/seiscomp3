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


#ifndef __SEISCOMP_CORE_SYSTEM_H__
#define __SEISCOMP_CORE_SYSTEM_H__

#include <cstdlib>
#include <string>

#include <seiscomp3/core/api.h>


namespace Seiscomp {
namespace Core {

SC_CORE_CORE_API std::string getLogin();
SC_CORE_CORE_API std::string getHostname();

SC_CORE_CORE_API void sleep(unsigned long seconds);
SC_CORE_CORE_API void msleep(unsigned long milliseconds);

SC_CORE_CORE_API unsigned int pid();

SC_CORE_CORE_API bool system(const std::string& command);

}
}


#endif
