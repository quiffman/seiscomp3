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


#ifndef __SEISCOMP_DATAMODEL_DATAUSED_H__
#define __SEISCOMP_DATAMODEL_DATAUSED_H__


#include <seiscomp3/datamodel/types.h>
#include <seiscomp3/datamodel/object.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(DataUsed);

class MomentTensor;


class SC_SYSTEM_CORE_API DataUsed : public Object {
	DECLARE_SC_CLASS(DataUsed);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		DataUsed();

		//! Copy constructor
		DataUsed(const DataUsed& other);

		//! Destructor
		~DataUsed();
	

	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		DataUsed& operator=(const DataUsed& other);
		//! Checks for equality of two objects. Childs objects
		//! are not part of the check.
		bool operator==(const DataUsed& other) const;
		bool operator!=(const DataUsed& other) const;

		//! Wrapper that calls operator==
		bool equal(const DataUsed& other) const;


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setWaveType(DataUsedWaveType waveType);
		DataUsedWaveType waveType() const;

		void setStationCount(int stationCount);
		int stationCount() const;

		void setComponentCount(int componentCount);
		int componentCount() const;

		void setShortestPeriod(const OPT(double)& shortestPeriod);
		double shortestPeriod() const throw(Seiscomp::Core::ValueException);

	
	// ------------------------------------------------------------------
	//  Public interface
	// ------------------------------------------------------------------
	public:
		MomentTensor* momentTensor() const;

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
		// Attributes
		DataUsedWaveType _waveType;
		int _stationCount;
		int _componentCount;
		OPT(double) _shortestPeriod;
};


}
}


#endif
