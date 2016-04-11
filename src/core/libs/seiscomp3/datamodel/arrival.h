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


#ifndef __SEISCOMP_DATAMODEL_ARRIVAL_H__
#define __SEISCOMP_DATAMODEL_ARRIVAL_H__


#include <seiscomp3/datamodel/creationinfo.h>
#include <string>
#include <seiscomp3/datamodel/phase.h>
#include <seiscomp3/datamodel/object.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(Arrival);

class Origin;


class SC_CORE_DATAMODEL_API ArrivalIndex {
	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		ArrivalIndex();
		ArrivalIndex(const std::string& pickID);

		//! Copy constructor
		ArrivalIndex(const ArrivalIndex&);


	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		bool operator==(const ArrivalIndex&) const;
		bool operator!=(const ArrivalIndex&) const;


	// ------------------------------------------------------------------
	//  Attributes
	// ------------------------------------------------------------------
	public:
		std::string pickID;
};


class SC_CORE_DATAMODEL_API Arrival : public Object {
	DECLARE_SC_CLASS(Arrival);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		Arrival();

		//! Copy constructor
		Arrival(const Arrival& other);

		//! Destructor
		~Arrival();
	

	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		Arrival& operator=(const Arrival& other);


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setPickID(const std::string& pickID);
		const std::string& pickID() const;

		void setPhase(const Phase& phase);
		Phase& phase();
		const Phase& phase() const;

		void setTimeCorrection(const OPT(double)& timeCorrection);
		double timeCorrection() const throw(Seiscomp::Core::ValueException);

		void setAzimuth(const OPT(double)& azimuth);
		double azimuth() const throw(Seiscomp::Core::ValueException);

		void setDistance(const OPT(double)& distance);
		double distance() const throw(Seiscomp::Core::ValueException);

		void setTakeOffAngle(const OPT(double)& takeOffAngle);
		double takeOffAngle() const throw(Seiscomp::Core::ValueException);

		void setTimeResidual(const OPT(double)& timeResidual);
		double timeResidual() const throw(Seiscomp::Core::ValueException);

		void setHorizontalSlownessResidual(const OPT(double)& horizontalSlownessResidual);
		double horizontalSlownessResidual() const throw(Seiscomp::Core::ValueException);

		void setBackazimuthResidual(const OPT(double)& backazimuthResidual);
		double backazimuthResidual() const throw(Seiscomp::Core::ValueException);

		void setTimeUsed(const OPT(bool)& timeUsed);
		bool timeUsed() const throw(Seiscomp::Core::ValueException);

		void setHorizontalSlownessUsed(const OPT(bool)& horizontalSlownessUsed);
		bool horizontalSlownessUsed() const throw(Seiscomp::Core::ValueException);

		void setBackazimuthUsed(const OPT(bool)& backazimuthUsed);
		bool backazimuthUsed() const throw(Seiscomp::Core::ValueException);

		void setWeight(const OPT(double)& weight);
		double weight() const throw(Seiscomp::Core::ValueException);

		void setEarthModelID(const std::string& earthModelID);
		const std::string& earthModelID() const;

		void setPreliminary(const OPT(bool)& preliminary);
		bool preliminary() const throw(Seiscomp::Core::ValueException);

		void setCreationInfo(const OPT(CreationInfo)& creationInfo);
		CreationInfo& creationInfo() throw(Seiscomp::Core::ValueException);
		const CreationInfo& creationInfo() const throw(Seiscomp::Core::ValueException);


	// ------------------------------------------------------------------
	//  Index management
	// ------------------------------------------------------------------
	public:
		//! Returns the object's index
		const ArrivalIndex& index() const;

		//! Checks two objects for equality regarding their index
		bool equalIndex(const Arrival* lhs) const;

	
	// ------------------------------------------------------------------
	//  Public interface
	// ------------------------------------------------------------------
	public:
		Origin* origin() const;

		//! Implement Object interface
		bool assign(Object* other);
		bool attachTo(PublicObject* parent);
		bool detachFrom(PublicObject* parent);
		bool detach();

		//! Creates a clone
		Object* clone() const;

		void accept(Visitor*);


	// ------------------------------------------------------------------
	//  Implementation
	// ------------------------------------------------------------------
	private:
		// Index
		ArrivalIndex _index;

		// Attributes
		Phase _phase;
		OPT(double) _timeCorrection;
		OPT(double) _azimuth;
		OPT(double) _distance;
		OPT(double) _takeOffAngle;
		OPT(double) _timeResidual;
		OPT(double) _horizontalSlownessResidual;
		OPT(double) _backazimuthResidual;
		OPT(bool) _timeUsed;
		OPT(bool) _horizontalSlownessUsed;
		OPT(bool) _backazimuthUsed;
		OPT(double) _weight;
		std::string _earthModelID;
		OPT(bool) _preliminary;
		OPT(CreationInfo) _creationInfo;
};


}
}


#endif
