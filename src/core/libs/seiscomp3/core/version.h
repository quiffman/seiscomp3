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


#ifndef __SC_CORE_VERSION_H__
#define __SC_CORE_VERSION_H__


#include <string>
#include <seiscomp3/core/api.h>


namespace Seiscomp {
namespace Core {


template <int major, int minor>
class VersionPacker {
	public:
		enum { Value = major << 0x10 | (minor & 0xFFFF) };
};


class SC_CORE_CORE_API Version {
	// ----------------------------------------------------------------------
	// Traits
	// ----------------------------------------------------------------------
	public:
		typedef unsigned int   PackType;
		typedef unsigned short TagType;


	// ----------------------------------------------------------------------
	// Xstruction
	// ----------------------------------------------------------------------
	public:
		//! C'tor
		Version(PackType packed_version = 0) : packed(packed_version) {}
		Version(TagType major, TagType minor) {
			packed = pack(major, minor);
		}


	// ----------------------------------------------------------------------
	// Public interface
	// ----------------------------------------------------------------------
	public:
		TagType majorTag() const { return packed >> 0x10; }
		TagType minorTag() const { return packed & 0xFFFF; }

		std::string toString() const;
		bool fromString(const std::string &str);

		static PackType pack(TagType major, TagType minor) {
			return major << 0x10 | (minor & 0xFFFF);
		}


	// ----------------------------------------------------------------------
	// Operators
	// ----------------------------------------------------------------------
	public:
		// Operators
		bool operator==(const Version &other) const { return packed == other.packed; }
		bool operator!=(const Version &other) const { return packed != other.packed; }
		bool operator<(const Version &other) const { return packed < other.packed; }
		bool operator>(const Version &other) const { return packed > other.packed; }
		bool operator<=(const Version &other) const { return packed <= other.packed; }
		bool operator>=(const Version &other) const { return packed >= other.packed; }


	// ----------------------------------------------------------------------
	// Public members
	// ----------------------------------------------------------------------
	public:
		PackType packed;
};


class SC_CORE_CORE_API FrameworkVersion {
	public:
		FrameworkVersion();

		//! Returns the version string
		std::string toString() const;

		//! Returns additional system build information
		std::string systemInfo() const;


	private:
		std::string _text;
};

extern FrameworkVersion CurrentVersion;


}
}


#endif
