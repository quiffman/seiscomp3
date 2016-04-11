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

// This file was created by a source code generator.
// Do not modify the contents. Change the definition and run the generator
// again!


#ifndef __SEISCOMP_DATAMODEL_WAVEFORMSTREAMID_H__
#define __SEISCOMP_DATAMODEL_WAVEFORMSTREAMID_H__


#include <string>
#include <seiscomp3/core/baseobject.h>
#include <seiscomp3/datamodel/api.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(WaveformStreamID);


class SC_CORE_DATAMODEL_API WaveformStreamID : public Core::BaseObject {
	DECLARE_SC_CLASS(WaveformStreamID);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		WaveformStreamID();

		//! Copy constructor
		WaveformStreamID(const WaveformStreamID& other);

		//! Custom constructor
		WaveformStreamID(const std::string& resourceURI);
		WaveformStreamID(const std::string& networkCode,
		                 const std::string& stationCode,
		                 const std::string& locationCode,
		                 const std::string& channelCode,
		                 const std::string& resourceURI);

		//! Destructor
		~WaveformStreamID();
	

	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		operator std::string&();
		operator const std::string&() const;

		//! Copies the metadata of other to this
		WaveformStreamID& operator=(const WaveformStreamID& other);
		//! Checks for equality of two objects. Childs objects
		//! are not part of the check.
		bool operator==(const WaveformStreamID& other) const;
		bool operator!=(const WaveformStreamID& other) const;

		//! Wrapper that calls operator==
		bool equal(const WaveformStreamID& other) const;


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setNetworkCode(const std::string& networkCode);
		const std::string& networkCode() const;

		void setStationCode(const std::string& stationCode);
		const std::string& stationCode() const;

		void setLocationCode(const std::string& locationCode);
		const std::string& locationCode() const;

		void setChannelCode(const std::string& channelCode);
		const std::string& channelCode() const;

		void setResourceURI(const std::string& resourceURI);
		const std::string& resourceURI() const;


	// ------------------------------------------------------------------
	//  Implementation
	// ------------------------------------------------------------------
	private:
		// Attributes
		std::string _networkCode;
		std::string _stationCode;
		std::string _locationCode;
		std::string _channelCode;
		std::string _resourceURI;
};


}
}


#endif
