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


#ifndef __SEISCOMP_DATAMODEL_DATALOGGER_H__
#define __SEISCOMP_DATAMODEL_DATALOGGER_H__


#include <seiscomp3/datamodel/blob.h>
#include <vector>
#include <string>
#include <seiscomp3/datamodel/dataloggercalibration.h>
#include <seiscomp3/datamodel/decimation.h>
#include <seiscomp3/datamodel/notifier.h>
#include <seiscomp3/datamodel/publicobject.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(Datalogger);
DEFINE_SMARTPOINTER(DataloggerCalibration);
DEFINE_SMARTPOINTER(Decimation);

class Inventory;


class SC_CORE_DATAMODEL_API DataloggerIndex {
	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		DataloggerIndex();
		DataloggerIndex(const std::string& name);

		//! Copy constructor
		DataloggerIndex(const DataloggerIndex&);


	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		bool operator==(const DataloggerIndex&) const;
		bool operator!=(const DataloggerIndex&) const;


	// ------------------------------------------------------------------
	//  Attributes
	// ------------------------------------------------------------------
	public:
		std::string name;
};


class SC_CORE_DATAMODEL_API Datalogger : public PublicObject {
	DECLARE_SC_CLASS(Datalogger);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	protected:
		//! Protected constructor
		Datalogger();

	public:
		//! Copy constructor
		Datalogger(const Datalogger& other);

		//! Constructor with publicID
		Datalogger(const std::string& publicID);

		//! Destructor
		~Datalogger();
	

	// ------------------------------------------------------------------
	//  Creators
	// ------------------------------------------------------------------
	public:
		static Datalogger* Create();
		static Datalogger* Create(const std::string& publicID);


	// ------------------------------------------------------------------
	//  Lookup
	// ------------------------------------------------------------------
	public:
		static Datalogger* Find(const std::string& publicID);


	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		//! No changes regarding child objects are made
		Datalogger& operator=(const Datalogger& other);


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setName(const std::string& name);
		const std::string& name() const;

		void setDescription(const std::string& description);
		const std::string& description() const;

		void setDigitizerModel(const std::string& digitizerModel);
		const std::string& digitizerModel() const;

		void setDigitizerManufacturer(const std::string& digitizerManufacturer);
		const std::string& digitizerManufacturer() const;

		void setRecorderModel(const std::string& recorderModel);
		const std::string& recorderModel() const;

		void setRecorderManufacturer(const std::string& recorderManufacturer);
		const std::string& recorderManufacturer() const;

		void setClockModel(const std::string& clockModel);
		const std::string& clockModel() const;

		void setClockManufacturer(const std::string& clockManufacturer);
		const std::string& clockManufacturer() const;

		void setClockType(const std::string& clockType);
		const std::string& clockType() const;

		void setGain(const OPT(double)& gain);
		double gain() const throw(Seiscomp::Core::ValueException);

		void setMaxClockDrift(const OPT(double)& maxClockDrift);
		double maxClockDrift() const throw(Seiscomp::Core::ValueException);

		void setRemark(const OPT(Blob)& remark);
		Blob& remark() throw(Seiscomp::Core::ValueException);
		const Blob& remark() const throw(Seiscomp::Core::ValueException);


	// ------------------------------------------------------------------
	//  Index management
	// ------------------------------------------------------------------
	public:
		//! Returns the object's index
		const DataloggerIndex& index() const;

		//! Checks two objects for equality regarding their index
		bool equalIndex(const Datalogger* lhs) const;

	
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
		bool add(DataloggerCalibration* obj);
		bool add(Decimation* obj);

		/**
		 * Removes an object.
		 * @param obj The object pointer
		 * @return true The object has been removed
		 * @return false The object has not been removed
		 *               because it does not exist in the list
		 */
		bool remove(DataloggerCalibration* obj);
		bool remove(Decimation* obj);

		/**
		 * Removes an object of a particular class.
		 * @param i The object index
		 * @return true The object has been removed
		 * @return false The index is out of bounds
		 */
		bool removeDataloggerCalibration(size_t i);
		bool removeDataloggerCalibration(const DataloggerCalibrationIndex& i);
		bool removeDecimation(size_t i);
		bool removeDecimation(const DecimationIndex& i);

		//! Retrieve the number of objects of a particular class
		size_t dataloggerCalibrationCount() const;
		size_t decimationCount() const;

		//! Index access
		//! @return The object at index i
		DataloggerCalibration* dataloggerCalibration(size_t i) const;
		DataloggerCalibration* dataloggerCalibration(const DataloggerCalibrationIndex& i) const;

		Decimation* decimation(size_t i) const;
		Decimation* decimation(const DecimationIndex& i) const;

		//! Find an object by its unique attribute(s)

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
		DataloggerIndex _index;

		// Attributes
		std::string _description;
		std::string _digitizerModel;
		std::string _digitizerManufacturer;
		std::string _recorderModel;
		std::string _recorderManufacturer;
		std::string _clockModel;
		std::string _clockManufacturer;
		std::string _clockType;
		OPT(double) _gain;
		OPT(double) _maxClockDrift;
		OPT(Blob) _remark;

		// Aggregations
		std::vector<DataloggerCalibrationPtr> _dataloggerCalibrations;
		std::vector<DecimationPtr> _decimations;

	DECLARE_SC_CLASSFACTORY_FRIEND(Datalogger);
};


}
}


#endif
