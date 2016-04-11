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


#ifndef __SEISCOMP_DATAMODEL_STATION_H__
#define __SEISCOMP_DATAMODEL_STATION_H__


#include <seiscomp3/datamodel/blob.h>
#include <seiscomp3/core/datetime.h>
#include <vector>
#include <string>
#include <seiscomp3/datamodel/sensorlocation.h>
#include <seiscomp3/datamodel/notifier.h>
#include <seiscomp3/datamodel/publicobject.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(Station);
DEFINE_SMARTPOINTER(SensorLocation);

class Network;


class SC_CORE_DATAMODEL_API StationIndex {
	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		StationIndex();
		StationIndex(const std::string& code,
		             Seiscomp::Core::Time start);

		//! Copy constructor
		StationIndex(const StationIndex&);


	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		bool operator==(const StationIndex&) const;
		bool operator!=(const StationIndex&) const;


	// ------------------------------------------------------------------
	//  Attributes
	// ------------------------------------------------------------------
	public:
		std::string code;
		Seiscomp::Core::Time start;
};


class SC_CORE_DATAMODEL_API Station : public PublicObject {
	DECLARE_SC_CLASS(Station);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	protected:
		//! Protected constructor
		Station();

	public:
		//! Copy constructor
		Station(const Station& other);

		//! Constructor with publicID
		Station(const std::string& publicID);

		//! Destructor
		~Station();
	

	// ------------------------------------------------------------------
	//  Creators
	// ------------------------------------------------------------------
	public:
		static Station* Create();
		static Station* Create(const std::string& publicID);


	// ------------------------------------------------------------------
	//  Lookup
	// ------------------------------------------------------------------
	public:
		static Station* Find(const std::string& publicID);


	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		//! No changes regarding child objects are made
		Station& operator=(const Station& other);


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

		void setLatitude(const OPT(double)& latitude);
		double latitude() const throw(Seiscomp::Core::ValueException);

		void setLongitude(const OPT(double)& longitude);
		double longitude() const throw(Seiscomp::Core::ValueException);

		void setElevation(const OPT(double)& elevation);
		double elevation() const throw(Seiscomp::Core::ValueException);

		void setPlace(const std::string& place);
		const std::string& place() const;

		void setCountry(const std::string& country);
		const std::string& country() const;

		void setAffiliation(const std::string& affiliation);
		const std::string& affiliation() const;

		void setType(const std::string& type);
		const std::string& type() const;

		void setArchive(const std::string& archive);
		const std::string& archive() const;

		void setArchiveNetworkCode(const std::string& archiveNetworkCode);
		const std::string& archiveNetworkCode() const;

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
		const StationIndex& index() const;

		//! Checks two objects for equality regarding their index
		bool equalIndex(const Station* lhs) const;

	
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
		bool add(SensorLocation* obj);

		/**
		 * Removes an object.
		 * @param obj The object pointer
		 * @return true The object has been removed
		 * @return false The object has not been removed
		 *               because it does not exist in the list
		 */
		bool remove(SensorLocation* obj);

		/**
		 * Removes an object of a particular class.
		 * @param i The object index
		 * @return true The object has been removed
		 * @return false The index is out of bounds
		 */
		bool removeSensorLocation(size_t i);
		bool removeSensorLocation(const SensorLocationIndex& i);

		//! Retrieve the number of objects of a particular class
		size_t sensorLocationCount() const;

		//! Index access
		//! @return The object at index i
		SensorLocation* sensorLocation(size_t i) const;
		SensorLocation* sensorLocation(const SensorLocationIndex& i) const;

		//! Find an object by its unique attribute(s)
		SensorLocation* findSensorLocation(const std::string& publicID) const;

		Network* network() const;

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
		StationIndex _index;

		// Attributes
		OPT(Seiscomp::Core::Time) _end;
		std::string _description;
		OPT(double) _latitude;
		OPT(double) _longitude;
		OPT(double) _elevation;
		std::string _place;
		std::string _country;
		std::string _affiliation;
		std::string _type;
		std::string _archive;
		std::string _archiveNetworkCode;
		OPT(bool) _restricted;
		OPT(bool) _shared;
		OPT(Blob) _remark;

		// Aggregations
		std::vector<SensorLocationPtr> _sensorLocations;

	DECLARE_SC_CLASSFACTORY_FRIEND(Station);
};


}
}


#endif
