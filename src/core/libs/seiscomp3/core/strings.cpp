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


#define SEISCOMP_COMPONENT Utils
#include <seiscomp3/core/strings.h>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <sstream>
#include <iostream>
#include <stdlib.h>
#include <cerrno>


namespace Seiscomp {
namespace Core {


namespace {

const char* timeFormat = "%FT%T.0000Z";
const char* timeFormatPrecise = "%FT%T.%fZ";

}


std::string toString(const std::string& value) {
	return value;
}



std::string toString(bool v) {
	return std::string(v?"true":"false");
}


std::string toString(time_t v) {
	return Seiscomp::Core::Time(v).toString(timeFormat);
}


std::string toString(const Seiscomp::Core::Time& v) {
	return v.toString(timeFormatPrecise);
}


std::string toString(const Enumeration& value) {
	return value.toString();
}


template <>
SC_CORE_CORE_API bool fromString(char& value, const std::string& str) {
	char* endptr = NULL;
	errno = 0;
	long int retval = strtol(str.c_str(), &endptr, 10);
	if ( errno != 0 )
		return false;
	if ( endptr ) {
		if ( str.c_str() + str.size() != endptr )
			return false;
		else if ( retval == 0 && str.c_str() == endptr )
			return false;
	}

	value = (char)retval;
	return true;
}


template <>
SC_CORE_CORE_API bool fromString(unsigned char& value, const std::string& str) {
	char* endptr = NULL;
	errno = 0;
	long int retval = strtol(str.c_str(), &endptr, 10);
	if ( errno != 0 )
		return false;
	if ( endptr ) {
		if ( str.c_str() + str.size() != endptr )
			return false;
		else if ( retval == 0 && str.c_str() == endptr )
			return false;
	}

	value = (unsigned char)retval;
	return true;
}


template <>
SC_CORE_CORE_API bool fromString(int& value, const std::string& str) {
	char* endptr = NULL;
	errno = 0;
	long int retval = strtol(str.c_str(), &endptr, 10);
	if ( errno != 0 )
		return false;
	if ( endptr ) {
		if ( str.c_str() + str.size() != endptr )
			return false;
		else if ( retval == 0 && str.c_str() == endptr )
			return false;
	}

	value = (int)retval;
	return true;
}


template <>
SC_CORE_CORE_API bool fromString(unsigned int& value, const std::string& str) {
	char* endptr = NULL;
	errno = 0;
	long int retval = strtol(str.c_str(), &endptr, 10);
	if ( errno != 0 )
		return false;
	if ( endptr ) {
		if ( str.c_str() + str.size() != endptr )
			return false;
		else if ( retval == 0 && str.c_str() == endptr )
			return false;
	}

	value = (unsigned int)retval;
	return true;
}


template <>
SC_CORE_CORE_API bool fromString(long& value, const std::string& str) {
	char* endptr = NULL;
	errno = 0;
	long int retval = strtol(str.c_str(), &endptr, 10);
	if ( errno != 0 )
		return false;
	if ( endptr ) {
		if ( str.c_str() + str.size() != endptr )
			return false;
		else if ( retval == 0 && str.c_str() == endptr )
			return false;
	}

	value = (long)retval;
	return true;
}


template <>
SC_CORE_CORE_API bool fromString(unsigned long& value, const std::string& str) {
	char* endptr = NULL;
	errno = 0;
	long int retval = strtol(str.c_str(), &endptr, 10);
	if ( errno != 0 )
		return false;
	if ( endptr ) {
		if ( str.c_str() + str.size() != endptr )
			return false;
		else if ( retval == 0 && str.c_str() == endptr )
			return false;
	}

	value = (unsigned long)retval;
	return true;
}


template <>
SC_CORE_CORE_API bool fromString(float& value, const std::string& str) {
	char* endptr = NULL;
	errno = 0;
	double retval = strtod(str.c_str(), &endptr);
	if ( errno != 0 )
		return false;
	if ( endptr ) {
		if ( str.c_str() + str.size() != endptr )
			return false;
		else if ( retval == 0 && str.c_str() == endptr )
			return false;
	}

	value = (float)retval;
	return true;
}


template <>
SC_CORE_CORE_API bool fromString(double& value, const std::string& str) {
	char* endptr = NULL;
	errno = 0;
	value = strtod(str.c_str(), &endptr);
	if ( errno != 0 )
		return false;
	if ( endptr ) {
		if ( str.c_str() + str.size() != endptr )
			return false;
		else if ( value == 0 && str.c_str() == endptr )
			return false;
	}

	return true;
}


template <>
SC_CORE_CORE_API bool fromString(bool& value, const std::string& str) {
	char* endptr = NULL;
	errno = 0;

	if ( compareNoCase(str, "true") == 0 ) {
		value = true;
		return true;
	}

	if ( compareNoCase(str, "false") == 0 ) {
		value = false;
		return true;
	}

	long int retval = strtol(str.c_str(), &endptr, 10);
	if ( errno != 0 )
		return false;
	if ( endptr ) {
		if ( str.c_str() + str.size() != endptr )
			return false;
		else if ( retval == 0 && str.c_str() == endptr )
			return false;
	}

	value = (bool)retval;
	return true;
}


bool fromString(time_t& value, const std::string& str) {
	Seiscomp::Core::Time t;
	if ( !t.fromString(str.c_str(), timeFormat) )
		return false;

	value = t.seconds();
	return true;
}


bool fromString(Time& value, const std::string& str) {
	return value.fromString(str.c_str(), timeFormatPrecise);
}


bool fromString(Enumeration& value, const std::string& str) {
	return value.fromString(str);
}


bool fromString(std::string& value, const std::string& str) {
	value.assign(str);
	return true;
}


int split(std::vector<std::string>& tokens, const char* source, const char* delimiter, bool compressOn) {
	boost::split(tokens, source, boost::is_any_of(delimiter),
	             ((compressOn) ? boost::token_compress_on : boost::token_compress_off));
	return static_cast<int>(tokens.size());
}


bool isEmpty(const char* str) {
	return str == NULL || *str == '\0';
}


int compareNoCase(const std::string& a, const std::string& b) {
	std::string::const_iterator it_a = a.begin(), it_b = b.begin();
	while ( it_a != a.end() && it_b != b.end() ) {
		char upper_a = toupper(*it_a);
		char upper_b = toupper(*it_b);
		if ( upper_a < upper_b )
			return -1;
		else if ( upper_a > upper_b )
			return 1;

		++it_a; ++it_b;
	}

	return it_a == a.end()?(it_b == b.end()?0:-1):(it_b == b.end()?1:0);
}


std::string& trim(std::string& str) {
// 	const char whitespace[] = "\t\n\v\f\r ";
//
// 	std::string::size_type pos;
// 	pos = str.find_first_not_of(whitespace);
// 	if (pos != 0) str.erase(0, pos);
//
// 	pos = str.find_last_not_of(whitespace);
// 	if (pos != std::string::npos) str.erase(pos + 1, std::string::npos);
	boost::trim(str);
	return str;
}


bool isWhitespace(const char c) {
	if ( WHITESPACE.find_first_of(c) == std::string::npos )
		return false;
	return true;
}


bool isWhitespace(const std::string& str) {
	std::string::const_iterator cIt = str.begin();
	for ( ; cIt != str.end(); ++cIt ) {
		if ( !isWhitespace(*cIt) )
			return false;
	}
	return true;
}


bool wildcmp(const char *wild, const char *str) {
	int slen = strlen(str);
	int wlen = strlen(wild);

	// if the compare string is too short, we're done
	int reqLen = 0;
	for (int i = 0; i < wlen; ++i)
		if (wild[i] != '*')
		++reqLen;

	if (slen < reqLen)
		return false;

	// length is okay; now we do the comparing
	int w = 0, s = 0;

	for (; s < slen && w < wlen; ++s, ++w) {
		// chars match; keep going
		if (wild[w] == str[s]) continue;

		// if we hit a '?' just go to the next char in both `str` and `wild`
		if (wild[w] == '?') continue;

		// we hit an unlimited wildcard
		if (wild[w] == '*') {
		// if it's the last char in the string, we're done
		if ((w + 1) == wlen)
			return true;

		bool ret = true;

		// for each remaining character in `wild`
		while (++w < wlen && ret) {

			// for each remaining character in `str`
			for (int i = 0; i < (slen - s); ++i) {
			// if the char is the same as the current `wild` char
			if (str[s + i] == wild[w]) {
				// compare from these points on
				ret = wildcmp(wild + w, str + s + i);
				// if successful, we're done
				if (ret)
				return true;
			}
			}
		}

		return ret;
		}
		// didn't hit a wildcard and chars don't match; failure
		else
		return false;
	}

	return /*(w >= wlen || ((w + 1) == wlen && wild[w] == _T('*'))) &&*/ s >= slen;
}


bool wildcmp(const std::string &wild, const std::string &str) {
	return wildcmp(wild.c_str(), str.c_str());
}


} // namespace Core
} // namespace Seiscomp
