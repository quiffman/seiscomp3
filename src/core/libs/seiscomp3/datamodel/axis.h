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


#ifndef __SEISCOMP_DATAMODEL_AXIS_H__
#define __SEISCOMP_DATAMODEL_AXIS_H__


#include <seiscomp3/datamodel/realquantity.h>
#include <seiscomp3/core/baseobject.h>
#include <seiscomp3/datamodel/api.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(Axis);


class SC_CORE_DATAMODEL_API Axis : public Core::BaseObject {
	DECLARE_SC_CLASS(Axis);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		Axis();

		//! Copy constructor
		Axis(const Axis& other);

		//! Destructor
		~Axis();
	

	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		Axis& operator=(const Axis& other);
		bool operator==(const Axis& other) const;
		bool operator!=(const Axis& other) const;


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setAzimuth(const RealQuantity& azimuth);
		RealQuantity& azimuth();
		const RealQuantity& azimuth() const;

		void setPlunge(const RealQuantity& plunge);
		RealQuantity& plunge();
		const RealQuantity& plunge() const;

		void setLength(const RealQuantity& length);
		RealQuantity& length();
		const RealQuantity& length() const;


	// ------------------------------------------------------------------
	//  Implementation
	// ------------------------------------------------------------------
	private:
		// Attributes
		RealQuantity _azimuth;
		RealQuantity _plunge;
		RealQuantity _length;
};


}
}


#endif
