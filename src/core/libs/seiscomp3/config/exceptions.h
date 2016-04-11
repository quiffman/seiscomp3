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


#ifndef __EXCEPTIONS_H__
#define __EXCEPTIONS_H__

#include <seiscomp3/core/exceptions.h>
#include <seiscomp3/config/api.h>

namespace Seiscomp {

class ConfigException : public Core::GeneralException {
	public:
		ConfigException() : Core::GeneralException("Configuration exception") { }
		ConfigException(const std::string& str) : Core::GeneralException(str) { }
};


class ConfigOptionNotFoundException : public ConfigException {
	public:
		ConfigOptionNotFoundException() : ConfigException("Option not found exception") { }
		ConfigOptionNotFoundException(const std::string& str) : ConfigException("Option not found exception for: " + str) { }
};


class ConfigTypeConversionException : public ConfigException {
	public:
		ConfigTypeConversionException() : ConfigException("Type conversion exception") { }
		ConfigTypeConversionException(const std::string& str) : ConfigException("Type conversion exception for: " + str) { }
};


//class SC_CORE_CONFIG_API OptionNotFoundException : public Core::GeneralException {
//	public:
//		OptionNotFoundException() : Core::GeneralException( "Option not found" ) {}
//		OptionNotFoundException( const std::string& str ) : Core::GeneralException( "Option not found: " + str ) {}
//};

} // namespace Seiscomp

#endif
