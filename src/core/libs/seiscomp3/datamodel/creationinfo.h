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


#ifndef __SEISCOMP_DATAMODEL_CREATIONINFO_H__
#define __SEISCOMP_DATAMODEL_CREATIONINFO_H__


#include <seiscomp3/core/datetime.h>
#include <string>
#include <seiscomp3/core/baseobject.h>
#include <seiscomp3/datamodel/api.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(CreationInfo);


class SC_CORE_DATAMODEL_API CreationInfo : public Core::BaseObject {
	DECLARE_SC_CLASS(CreationInfo);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		CreationInfo();

		//! Copy constructor
		CreationInfo(const CreationInfo& other);

		//! Destructor
		~CreationInfo();
	

	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		CreationInfo& operator=(const CreationInfo& other);
		//! Checks for equality of two objects. Childs objects
		//! are not part of the check.
		bool operator==(const CreationInfo& other) const;
		bool operator!=(const CreationInfo& other) const;

		//! Wrapper that calls operator==
		bool equal(const CreationInfo& other) const;


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setAgencyID(const std::string& agencyID);
		const std::string& agencyID() const;

		void setAgencyURI(const std::string& agencyURI);
		const std::string& agencyURI() const;

		void setAuthor(const std::string& author);
		const std::string& author() const;

		void setAuthorURI(const std::string& authorURI);
		const std::string& authorURI() const;

		void setCreationTime(const OPT(Seiscomp::Core::Time)& creationTime);
		Seiscomp::Core::Time creationTime() const throw(Seiscomp::Core::ValueException);

		void setModificationTime(const OPT(Seiscomp::Core::Time)& modificationTime);
		Seiscomp::Core::Time modificationTime() const throw(Seiscomp::Core::ValueException);

		void setVersion(const std::string& version);
		const std::string& version() const;


	// ------------------------------------------------------------------
	//  Implementation
	// ------------------------------------------------------------------
	private:
		// Attributes
		std::string _agencyID;
		std::string _agencyURI;
		std::string _author;
		std::string _authorURI;
		OPT(Seiscomp::Core::Time) _creationTime;
		OPT(Seiscomp::Core::Time) _modificationTime;
		std::string _version;
};


}
}


#endif
