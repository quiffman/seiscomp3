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


#ifndef __SEISCOMP_DATAMODEL_CONFIDENCEELLIPSOID_H__
#define __SEISCOMP_DATAMODEL_CONFIDENCEELLIPSOID_H__


#include <seiscomp3/core/baseobject.h>
#include <seiscomp3/core.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(ConfidenceEllipsoid);


class SC_SYSTEM_CORE_API ConfidenceEllipsoid : public Core::BaseObject {
	DECLARE_SC_CLASS(ConfidenceEllipsoid);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		ConfidenceEllipsoid();

		//! Copy constructor
		ConfidenceEllipsoid(const ConfidenceEllipsoid& other);

		//! Destructor
		~ConfidenceEllipsoid();
	

	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		ConfidenceEllipsoid& operator=(const ConfidenceEllipsoid& other);
		//! Checks for equality of two objects. Childs objects
		//! are not part of the check.
		bool operator==(const ConfidenceEllipsoid& other) const;
		bool operator!=(const ConfidenceEllipsoid& other) const;

		//! Wrapper that calls operator==
		bool equal(const ConfidenceEllipsoid& other) const;


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setSemiMajorAxisLength(double semiMajorAxisLength);
		double semiMajorAxisLength() const;

		void setSemiMinorAxisLength(double semiMinorAxisLength);
		double semiMinorAxisLength() const;

		void setSemiIntermediateAxisLength(double semiIntermediateAxisLength);
		double semiIntermediateAxisLength() const;

		void setMajorAxisPlunge(double majorAxisPlunge);
		double majorAxisPlunge() const;

		void setMajorAxisAzimuth(double majorAxisAzimuth);
		double majorAxisAzimuth() const;

		void setMajorAxisRotation(double majorAxisRotation);
		double majorAxisRotation() const;


	// ------------------------------------------------------------------
	//  Implementation
	// ------------------------------------------------------------------
	private:
		// Attributes
		double _semiMajorAxisLength;
		double _semiMinorAxisLength;
		double _semiIntermediateAxisLength;
		double _majorAxisPlunge;
		double _majorAxisAzimuth;
		double _majorAxisRotation;
};


}
}


#endif
