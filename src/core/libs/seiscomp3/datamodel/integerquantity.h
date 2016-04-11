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


#ifndef __SEISCOMP_DATAMODEL_INTEGERQUANTITY_H__
#define __SEISCOMP_DATAMODEL_INTEGERQUANTITY_H__


#include <seiscomp3/core/baseobject.h>
#include <seiscomp3/datamodel/api.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(IntegerQuantity);


class SC_CORE_DATAMODEL_API IntegerQuantity : public Core::BaseObject {
	DECLARE_SC_CLASS(IntegerQuantity);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		IntegerQuantity();

		//! Copy constructor
		IntegerQuantity(const IntegerQuantity& other);

		//! Custom constructor
		IntegerQuantity(int value);
		IntegerQuantity(int value,
		                const OPT(int)& uncertainty,
		                const OPT(int)& lowerUncertainty,
		                const OPT(int)& upperUncertainty,
		                const OPT(double)& confidenceLevel);

		//! Destructor
		~IntegerQuantity();
	

	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		operator int&();
		operator int() const;

		//! Copies the metadata of other to this
		IntegerQuantity& operator=(const IntegerQuantity& other);
		//! Checks for equality of two objects. Childs objects
		//! are not part of the check.
		bool operator==(const IntegerQuantity& other) const;
		bool operator!=(const IntegerQuantity& other) const;

		//! Wrapper that calls operator==
		bool equal(const IntegerQuantity& other) const;


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setValue(int value);
		int value() const;

		void setUncertainty(const OPT(int)& uncertainty);
		int uncertainty() const throw(Seiscomp::Core::ValueException);

		void setLowerUncertainty(const OPT(int)& lowerUncertainty);
		int lowerUncertainty() const throw(Seiscomp::Core::ValueException);

		void setUpperUncertainty(const OPT(int)& upperUncertainty);
		int upperUncertainty() const throw(Seiscomp::Core::ValueException);

		void setConfidenceLevel(const OPT(double)& confidenceLevel);
		double confidenceLevel() const throw(Seiscomp::Core::ValueException);


	// ------------------------------------------------------------------
	//  Implementation
	// ------------------------------------------------------------------
	private:
		// Attributes
		int _value;
		OPT(int) _uncertainty;
		OPT(int) _lowerUncertainty;
		OPT(int) _upperUncertainty;
		OPT(double) _confidenceLevel;
};


}
}


#endif
