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


#ifndef __SEISCOMP_DATAMODEL_STATIONMAGNITUDECONTRIBUTION_H__
#define __SEISCOMP_DATAMODEL_STATIONMAGNITUDECONTRIBUTION_H__


#include <string>
#include <seiscomp3/datamodel/object.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(StationMagnitudeContribution);

class Magnitude;


class SC_SYSTEM_CORE_API StationMagnitudeContributionIndex {
	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		StationMagnitudeContributionIndex();
		StationMagnitudeContributionIndex(const std::string& stationMagnitudeID);

		//! Copy constructor
		StationMagnitudeContributionIndex(const StationMagnitudeContributionIndex&);


	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		bool operator==(const StationMagnitudeContributionIndex&) const;
		bool operator!=(const StationMagnitudeContributionIndex&) const;


	// ------------------------------------------------------------------
	//  Attributes
	// ------------------------------------------------------------------
	public:
		std::string stationMagnitudeID;
};


class SC_SYSTEM_CORE_API StationMagnitudeContribution : public Object {
	DECLARE_SC_CLASS(StationMagnitudeContribution);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		StationMagnitudeContribution();

		//! Copy constructor
		StationMagnitudeContribution(const StationMagnitudeContribution& other);

		//! Custom constructor
		StationMagnitudeContribution(const std::string& stationMagnitudeID);
		StationMagnitudeContribution(const std::string& stationMagnitudeID,
		                             const OPT(double)& residual,
		                             const OPT(double)& weight);

		//! Destructor
		~StationMagnitudeContribution();
	

	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		StationMagnitudeContribution& operator=(const StationMagnitudeContribution& other);
		//! Checks for equality of two objects. Childs objects
		//! are not part of the check.
		bool operator==(const StationMagnitudeContribution& other) const;
		bool operator!=(const StationMagnitudeContribution& other) const;

		//! Wrapper that calls operator==
		bool equal(const StationMagnitudeContribution& other) const;


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setStationMagnitudeID(const std::string& stationMagnitudeID);
		const std::string& stationMagnitudeID() const;

		void setResidual(const OPT(double)& residual);
		double residual() const throw(Seiscomp::Core::ValueException);

		void setWeight(const OPT(double)& weight);
		double weight() const throw(Seiscomp::Core::ValueException);


	// ------------------------------------------------------------------
	//  Index management
	// ------------------------------------------------------------------
	public:
		//! Returns the object's index
		const StationMagnitudeContributionIndex& index() const;

		//! Checks two objects for equality regarding their index
		bool equalIndex(const StationMagnitudeContribution* lhs) const;

	
	// ------------------------------------------------------------------
	//  Public interface
	// ------------------------------------------------------------------
	public:
		Magnitude* magnitude() const;

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
		StationMagnitudeContributionIndex _index;

		// Attributes
		OPT(double) _residual;
		OPT(double) _weight;
};


}
}


#endif
