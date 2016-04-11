/***************************************************************************
 *   Copyright (C) 2009 by gempa GmbH
 *
 *   Author: Jan Becker
 *   Email: jabe@gempa.de $
 *
 ***************************************************************************/


#ifndef __SEISCOMP_STATIONXML_POLEANDZERO_H__
#define __SEISCOMP_STATIONXML_POLEANDZERO_H__


#include <stationxml/metadata.h>
#include <stationxml/floatnounittype.h>
#include <seiscomp3/core/baseobject.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace StationXML {


DEFINE_SMARTPOINTER(PoleAndZero);


/**
 * \brief Complex numbers used as poles or zeros in channel response.
 */
class PoleAndZero : public Core::BaseObject {
	DECLARE_CASTS(PoleAndZero);
	DECLARE_RTTI;
	DECLARE_METAOBJECT_DERIVED;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		PoleAndZero();

		//! Copy constructor
		PoleAndZero(const PoleAndZero& other);

		//! Destructor
		~PoleAndZero();


	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		PoleAndZero& operator=(const PoleAndZero& other);
		bool operator==(const PoleAndZero& other) const;


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		//! XML tag: Real
		void setReal(const FloatNoUnitType& real);
		FloatNoUnitType& real();
		const FloatNoUnitType& real() const;

		//! XML tag: Imaginary
		void setImaginary(const FloatNoUnitType& imaginary);
		FloatNoUnitType& imaginary();
		const FloatNoUnitType& imaginary() const;

		//! XML tag: number
		void setNumber(int number);
		int number() const;

	
	// ------------------------------------------------------------------
	//  Public interface
	// ------------------------------------------------------------------
	public:

	// ------------------------------------------------------------------
	//  Implementation
	// ------------------------------------------------------------------
	private:
		// Attributes
		FloatNoUnitType _real;
		FloatNoUnitType _imaginary;
		int _number;
};


}
}


#endif
