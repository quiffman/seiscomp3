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


#ifndef __SEISCOMP_DATAMODEL_COMPOSITETIME_H__
#define __SEISCOMP_DATAMODEL_COMPOSITETIME_H__


#include <seiscomp3/datamodel/integerquantity.h>
#include <seiscomp3/datamodel/realquantity.h>
#include <seiscomp3/datamodel/object.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(CompositeTime);

class Origin;


class SC_CORE_DATAMODEL_API CompositeTime : public Object {
	DECLARE_SC_CLASS(CompositeTime);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		CompositeTime();

		//! Copy constructor
		CompositeTime(const CompositeTime& other);

		//! Destructor
		~CompositeTime();
	

	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		CompositeTime& operator=(const CompositeTime& other);
		//! Checks for equality of two objects. Childs objects
		//! are not part of the check.
		bool operator==(const CompositeTime& other) const;
		bool operator!=(const CompositeTime& other) const;

		//! Wrapper that calls operator==
		bool equal(const CompositeTime& other) const;


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setYear(const OPT(IntegerQuantity)& year);
		IntegerQuantity& year() throw(Seiscomp::Core::ValueException);
		const IntegerQuantity& year() const throw(Seiscomp::Core::ValueException);

		void setMonth(const OPT(IntegerQuantity)& month);
		IntegerQuantity& month() throw(Seiscomp::Core::ValueException);
		const IntegerQuantity& month() const throw(Seiscomp::Core::ValueException);

		void setDay(const OPT(IntegerQuantity)& day);
		IntegerQuantity& day() throw(Seiscomp::Core::ValueException);
		const IntegerQuantity& day() const throw(Seiscomp::Core::ValueException);

		void setHour(const OPT(IntegerQuantity)& hour);
		IntegerQuantity& hour() throw(Seiscomp::Core::ValueException);
		const IntegerQuantity& hour() const throw(Seiscomp::Core::ValueException);

		void setMinute(const OPT(IntegerQuantity)& minute);
		IntegerQuantity& minute() throw(Seiscomp::Core::ValueException);
		const IntegerQuantity& minute() const throw(Seiscomp::Core::ValueException);

		void setSecond(const OPT(RealQuantity)& second);
		RealQuantity& second() throw(Seiscomp::Core::ValueException);
		const RealQuantity& second() const throw(Seiscomp::Core::ValueException);

	
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
		// Attributes
		OPT(IntegerQuantity) _year;
		OPT(IntegerQuantity) _month;
		OPT(IntegerQuantity) _day;
		OPT(IntegerQuantity) _hour;
		OPT(IntegerQuantity) _minute;
		OPT(RealQuantity) _second;
};


}
}


#endif
