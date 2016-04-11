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


#ifndef __SEISCOMP_DATAMODEL_NODALPLANES_H__
#define __SEISCOMP_DATAMODEL_NODALPLANES_H__


#include <seiscomp3/datamodel/nodalplane.h>
#include <seiscomp3/core/baseobject.h>
#include <seiscomp3/datamodel/api.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(NodalPlanes);


class SC_CORE_DATAMODEL_API NodalPlanes : public Core::BaseObject {
	DECLARE_SC_CLASS(NodalPlanes);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		NodalPlanes();

		//! Copy constructor
		NodalPlanes(const NodalPlanes& other);

		//! Destructor
		~NodalPlanes();
	

	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		NodalPlanes& operator=(const NodalPlanes& other);
		bool operator==(const NodalPlanes& other) const;
		bool operator!=(const NodalPlanes& other) const;


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setNodalPlane1(const OPT(NodalPlane)& nodalPlane1);
		NodalPlane& nodalPlane1() throw(Seiscomp::Core::ValueException);
		const NodalPlane& nodalPlane1() const throw(Seiscomp::Core::ValueException);

		void setNodalPlane2(const OPT(NodalPlane)& nodalPlane2);
		NodalPlane& nodalPlane2() throw(Seiscomp::Core::ValueException);
		const NodalPlane& nodalPlane2() const throw(Seiscomp::Core::ValueException);

		void setPreferredPlane(const OPT(int)& preferredPlane);
		int preferredPlane() const throw(Seiscomp::Core::ValueException);


	// ------------------------------------------------------------------
	//  Implementation
	// ------------------------------------------------------------------
	private:
		// Attributes
		OPT(NodalPlane) _nodalPlane1;
		OPT(NodalPlane) _nodalPlane2;
		OPT(int) _preferredPlane;
};


}
}


#endif
