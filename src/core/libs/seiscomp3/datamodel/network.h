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


#ifndef __SEISCOMP_DATAMODEL_NETWORK_H__
#define __SEISCOMP_DATAMODEL_NETWORK_H__


#include <seiscomp3/datamodel/blob.h>
#include <seiscomp3/core/datetime.h>
#include <vector>
#include <string>
#include <seiscomp3/datamodel/station.h>
#include <seiscomp3/datamodel/notifier.h>
#include <seiscomp3/datamodel/publicobject.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(Network);
DEFINE_SMARTPOINTER(Station);

class Inventory;


class SC_CORE_DATAMODEL_API NetworkIndex {
	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		NetworkIndex();
		NetworkIndex(const std::string& code,
		             Seiscomp::Core::Time start);

		//! Copy constructor
		NetworkIndex(const NetworkIndex&);


	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		bool operator==(const NetworkIndex&) const;
		bool operator!=(const NetworkIndex&) const;


	// ------------------------------------------------------------------
	//  Attributes
	// ------------------------------------------------------------------
	public:
		std::string code;
		Seiscomp::Core::Time start;
};


class SC_CORE_DATAMODEL_API Network : public PublicObject {
	DECLARE_SC_CLASS(Network);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	protected:
		//! Protected constructor
		Network();

	public:
		//! Copy constructor
		Network(const Network& other);

		//! Constructor with publicID
		Network(const std::string& publicID);

		//! Destructor
		~Network();
	

	// ------------------------------------------------------------------
	//  Creators
	// ------------------------------------------------------------------
	public:
		static Network* Create();
		static Network* Create(const std::string& publicID);


	// ------------------------------------------------------------------
	//  Lookup
	// ------------------------------------------------------------------
	public:
		static Network* Find(const std::string& publicID);


	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		//! No changes regarding child objects are made
		Network& operator=(const Network& other);


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setCode(const std::string& code);
		const std::string& code() const;

		void setStart(Seiscomp::Core::Time start);
		Seiscomp::Core::Time start() const;

		void setEnd(const OPT(Seiscomp::Core::Time)& end);
		Seiscomp::Core::Time end() const throw(Seiscomp::Core::ValueException);

		void setDescription(const std::string& description);
		const std::string& description() const;

		void setInstitutions(const std::string& institutions);
		const std::string& institutions() const;

		void setRegion(const std::string& region);
		const std::string& region() const;

		void setType(const std::string& type);
		const std::string& type() const;

		void setNetClass(const std::string& netClass);
		const std::string& netClass() const;

		void setArchive(const std::string& archive);
		const std::string& archive() const;

		void setRestricted(const OPT(bool)& restricted);
		bool restricted() const throw(Seiscomp::Core::ValueException);

		void setShared(const OPT(bool)& shared);
		bool shared() const throw(Seiscomp::Core::ValueException);

		void setRemark(const OPT(Blob)& remark);
		Blob& remark() throw(Seiscomp::Core::ValueException);
		const Blob& remark() const throw(Seiscomp::Core::ValueException);


	// ------------------------------------------------------------------
	//  Index management
	// ------------------------------------------------------------------
	public:
		//! Returns the object's index
		const NetworkIndex& index() const;

		//! Checks two objects for equality regarding their index
		bool equalIndex(const Network* lhs) const;

	
	// ------------------------------------------------------------------
	//  Public interface
	// ------------------------------------------------------------------
	public:
		/**
		 * Add an object.
		 * @param obj The object pointer
		 * @return true The object has been added
		 * @return false The object has not been added
		 *               because it already exists in the list
		 *               or it already has another parent
		 */
		bool add(Station* obj);

		/**
		 * Removes an object.
		 * @param obj The object pointer
		 * @return true The object has been removed
		 * @return false The object has not been removed
		 *               because it does not exist in the list
		 */
		bool remove(Station* obj);

		/**
		 * Removes an object of a particular class.
		 * @param i The object index
		 * @return true The object has been removed
		 * @return false The index is out of bounds
		 */
		bool removeStation(size_t i);
		bool removeStation(const StationIndex& i);

		//! Retrieve the number of objects of a particular class
		size_t stationCount() const;

		//! Index access
		//! @return The object at index i
		Station* station(size_t i) const;
		Station* station(const StationIndex& i) const;

		//! Find an object by its unique attribute(s)
		Station* findStation(const std::string& publicID) const;

		Inventory* inventory() const;

		//! Implement Object interface
		bool assign(Object* other);
		bool attachTo(PublicObject* parent);
		bool detachFrom(PublicObject* parent);
		bool detach();

		//! Creates a clone
		Object* clone() const;

		//! Implement PublicObject interface
		bool updateChild(Object* child);

		void accept(Visitor*);


	// ------------------------------------------------------------------
	//  Implementation
	// ------------------------------------------------------------------
	private:
		// Index
		NetworkIndex _index;

		// Attributes
		OPT(Seiscomp::Core::Time) _end;
		std::string _description;
		std::string _institutions;
		std::string _region;
		std::string _type;
		std::string _netClass;
		std::string _archive;
		OPT(bool) _restricted;
		OPT(bool) _shared;
		OPT(Blob) _remark;

		// Aggregations
		std::vector<StationPtr> _stations;

	DECLARE_SC_CLASSFACTORY_FRIEND(Network);
};


}
}


#endif
