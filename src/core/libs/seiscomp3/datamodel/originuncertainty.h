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


#ifndef __SEISCOMP_DATAMODEL_ORIGINUNCERTAINTY_H__
#define __SEISCOMP_DATAMODEL_ORIGINUNCERTAINTY_H__


#include <seiscomp3/datamodel/confidenceellipsoid.h>
#include <seiscomp3/datamodel/types.h>
#include <seiscomp3/core/baseobject.h>
#include <seiscomp3/datamodel/api.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(OriginUncertainty);


class SC_CORE_DATAMODEL_API OriginUncertainty : public Core::BaseObject {
	DECLARE_SC_CLASS(OriginUncertainty);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		OriginUncertainty();

		//! Copy constructor
		OriginUncertainty(const OriginUncertainty& other);

		//! Destructor
		~OriginUncertainty();
	

	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		OriginUncertainty& operator=(const OriginUncertainty& other);
		//! Checks for equality of two objects. Childs objects
		//! are not part of the check.
		bool operator==(const OriginUncertainty& other) const;
		bool operator!=(const OriginUncertainty& other) const;

		//! Wrapper that calls operator==
		bool equal(const OriginUncertainty& other) const;


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setHorizontalUncertainty(const OPT(double)& horizontalUncertainty);
		double horizontalUncertainty() const throw(Seiscomp::Core::ValueException);

		void setMinHorizontalUncertainty(const OPT(double)& minHorizontalUncertainty);
		double minHorizontalUncertainty() const throw(Seiscomp::Core::ValueException);

		void setMaxHorizontalUncertainty(const OPT(double)& maxHorizontalUncertainty);
		double maxHorizontalUncertainty() const throw(Seiscomp::Core::ValueException);

		void setAzimuthMaxHorizontalUncertainty(const OPT(double)& azimuthMaxHorizontalUncertainty);
		double azimuthMaxHorizontalUncertainty() const throw(Seiscomp::Core::ValueException);

		void setConfidenceEllipsoid(const OPT(ConfidenceEllipsoid)& confidenceEllipsoid);
		ConfidenceEllipsoid& confidenceEllipsoid() throw(Seiscomp::Core::ValueException);
		const ConfidenceEllipsoid& confidenceEllipsoid() const throw(Seiscomp::Core::ValueException);

		void setPreferredDescription(const OPT(OriginUncertaintyDescription)& preferredDescription);
		OriginUncertaintyDescription preferredDescription() const throw(Seiscomp::Core::ValueException);


	// ------------------------------------------------------------------
	//  Implementation
	// ------------------------------------------------------------------
	private:
		// Attributes
		OPT(double) _horizontalUncertainty;
		OPT(double) _minHorizontalUncertainty;
		OPT(double) _maxHorizontalUncertainty;
		OPT(double) _azimuthMaxHorizontalUncertainty;
		OPT(ConfidenceEllipsoid) _confidenceEllipsoid;
		OPT(OriginUncertaintyDescription) _preferredDescription;
};


}
}


#endif
