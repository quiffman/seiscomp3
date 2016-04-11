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


#ifndef __SEISCOMP_DATAMODEL_NODALPLANE_H__
#define __SEISCOMP_DATAMODEL_NODALPLANE_H__


#include <seiscomp3/datamodel/realquantity.h>
#include <seiscomp3/core/baseobject.h>
#include <seiscomp3/datamodel/api.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(NodalPlane);


class SC_CORE_DATAMODEL_API NodalPlane : public Core::BaseObject {
	DECLARE_SC_CLASS(NodalPlane);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		NodalPlane();

		//! Copy constructor
		NodalPlane(const NodalPlane& other);

		//! Destructor
		~NodalPlane();
	

	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		NodalPlane& operator=(const NodalPlane& other);
		bool operator==(const NodalPlane& other) const;
		bool operator!=(const NodalPlane& other) const;


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setStrike(const RealQuantity& strike);
		RealQuantity& strike();
		const RealQuantity& strike() const;

		void setDip(const RealQuantity& dip);
		RealQuantity& dip();
		const RealQuantity& dip() const;

		void setRake(const RealQuantity& rake);
		RealQuantity& rake();
		const RealQuantity& rake() const;


	// ------------------------------------------------------------------
	//  Implementation
	// ------------------------------------------------------------------
	private:
		// Attributes
		RealQuantity _strike;
		RealQuantity _dip;
		RealQuantity _rake;
};


}
}


#endif
