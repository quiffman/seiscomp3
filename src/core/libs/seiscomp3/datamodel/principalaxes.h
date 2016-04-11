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


#ifndef __SEISCOMP_DATAMODEL_PRINCIPALAXES_H__
#define __SEISCOMP_DATAMODEL_PRINCIPALAXES_H__


#include <seiscomp3/datamodel/axis.h>
#include <seiscomp3/core/baseobject.h>
#include <seiscomp3/datamodel/api.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(PrincipalAxes);


class SC_CORE_DATAMODEL_API PrincipalAxes : public Core::BaseObject {
	DECLARE_SC_CLASS(PrincipalAxes);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		PrincipalAxes();

		//! Copy constructor
		PrincipalAxes(const PrincipalAxes& other);

		//! Destructor
		~PrincipalAxes();
	

	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		PrincipalAxes& operator=(const PrincipalAxes& other);
		bool operator==(const PrincipalAxes& other) const;
		bool operator!=(const PrincipalAxes& other) const;


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setTAxis(const Axis& tAxis);
		Axis& tAxis();
		const Axis& tAxis() const;

		void setPAxis(const Axis& pAxis);
		Axis& pAxis();
		const Axis& pAxis() const;

		void setNAxis(const OPT(Axis)& nAxis);
		Axis& nAxis() throw(Seiscomp::Core::ValueException);
		const Axis& nAxis() const throw(Seiscomp::Core::ValueException);


	// ------------------------------------------------------------------
	//  Implementation
	// ------------------------------------------------------------------
	private:
		// Attributes
		Axis _tAxis;
		Axis _pAxis;
		OPT(Axis) _nAxis;
};


}
}


#endif
