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


#ifndef __SEISCOMP_DATAMODEL_REALQUANTITY_H__
#define __SEISCOMP_DATAMODEL_REALQUANTITY_H__


#include <seiscomp3/core/baseobject.h>
#include <seiscomp3/datamodel/api.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(RealQuantity);


class SC_CORE_DATAMODEL_API RealQuantity : public Core::BaseObject {
	DECLARE_SC_CLASS(RealQuantity);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		RealQuantity();

		//! Copy constructor
		RealQuantity(const RealQuantity& other);

		//! Custom constructor
		RealQuantity(double value);
		RealQuantity(double value,
		             const OPT(double)& uncertainty,
		             const OPT(double)& lowerUncertainty,
		             const OPT(double)& upperUncertainty,
		             const OPT(double)& confidenceLevel);

		//! Destructor
		~RealQuantity();
	

	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		operator double&();
		operator double() const;

		//! Copies the metadata of other to this
		RealQuantity& operator=(const RealQuantity& other);
		//! Checks for equality of two objects. Childs objects
		//! are not part of the check.
		bool operator==(const RealQuantity& other) const;
		bool operator!=(const RealQuantity& other) const;

		//! Wrapper that calls operator==
		bool equal(const RealQuantity& other) const;


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setValue(double value);
		double value() const;

		void setUncertainty(const OPT(double)& uncertainty);
		double uncertainty() const throw(Seiscomp::Core::ValueException);

		void setLowerUncertainty(const OPT(double)& lowerUncertainty);
		double lowerUncertainty() const throw(Seiscomp::Core::ValueException);

		void setUpperUncertainty(const OPT(double)& upperUncertainty);
		double upperUncertainty() const throw(Seiscomp::Core::ValueException);

		void setConfidenceLevel(const OPT(double)& confidenceLevel);
		double confidenceLevel() const throw(Seiscomp::Core::ValueException);


	// ------------------------------------------------------------------
	//  Implementation
	// ------------------------------------------------------------------
	private:
		// Attributes
		double _value;
		OPT(double) _uncertainty;
		OPT(double) _lowerUncertainty;
		OPT(double) _upperUncertainty;
		OPT(double) _confidenceLevel;
};


}
}


#endif
