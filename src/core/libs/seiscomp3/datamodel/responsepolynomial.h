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


#ifndef __SEISCOMP_DATAMODEL_RESPONSEPOLYNOMIAL_H__
#define __SEISCOMP_DATAMODEL_RESPONSEPOLYNOMIAL_H__


#include <seiscomp3/datamodel/blob.h>
#include <string>
#include <seiscomp3/datamodel/realarray.h>
#include <seiscomp3/datamodel/publicobject.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(ResponsePolynomial);

class Inventory;


class SC_CORE_DATAMODEL_API ResponsePolynomialIndex {
	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		ResponsePolynomialIndex();
		ResponsePolynomialIndex(const std::string& name);

		//! Copy constructor
		ResponsePolynomialIndex(const ResponsePolynomialIndex&);


	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		bool operator==(const ResponsePolynomialIndex&) const;
		bool operator!=(const ResponsePolynomialIndex&) const;


	// ------------------------------------------------------------------
	//  Attributes
	// ------------------------------------------------------------------
	public:
		std::string name;
};


class SC_CORE_DATAMODEL_API ResponsePolynomial : public PublicObject {
	DECLARE_SC_CLASS(ResponsePolynomial);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	protected:
		//! Protected constructor
		ResponsePolynomial();

	public:
		//! Copy constructor
		ResponsePolynomial(const ResponsePolynomial& other);

		//! Constructor with publicID
		ResponsePolynomial(const std::string& publicID);

		//! Destructor
		~ResponsePolynomial();
	

	// ------------------------------------------------------------------
	//  Creators
	// ------------------------------------------------------------------
	public:
		static ResponsePolynomial* Create();
		static ResponsePolynomial* Create(const std::string& publicID);


	// ------------------------------------------------------------------
	//  Lookup
	// ------------------------------------------------------------------
	public:
		static ResponsePolynomial* Find(const std::string& publicID);


	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		//! No changes regarding child objects are made
		ResponsePolynomial& operator=(const ResponsePolynomial& other);


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setName(const std::string& name);
		const std::string& name() const;

		void setGain(const OPT(double)& gain);
		double gain() const throw(Seiscomp::Core::ValueException);

		void setGainFrequency(const OPT(double)& gainFrequency);
		double gainFrequency() const throw(Seiscomp::Core::ValueException);

		void setFrequencyUnit(const std::string& frequencyUnit);
		const std::string& frequencyUnit() const;

		void setApproximationType(const std::string& approximationType);
		const std::string& approximationType() const;

		void setApproximationLowerBound(const OPT(double)& approximationLowerBound);
		double approximationLowerBound() const throw(Seiscomp::Core::ValueException);

		void setApproximationUpperBound(const OPT(double)& approximationUpperBound);
		double approximationUpperBound() const throw(Seiscomp::Core::ValueException);

		void setApproximationError(const OPT(double)& approximationError);
		double approximationError() const throw(Seiscomp::Core::ValueException);

		void setNumberOfCoefficients(const OPT(int)& numberOfCoefficients);
		int numberOfCoefficients() const throw(Seiscomp::Core::ValueException);

		void setCoefficients(const OPT(RealArray)& coefficients);
		RealArray& coefficients() throw(Seiscomp::Core::ValueException);
		const RealArray& coefficients() const throw(Seiscomp::Core::ValueException);

		void setRemark(const OPT(Blob)& remark);
		Blob& remark() throw(Seiscomp::Core::ValueException);
		const Blob& remark() const throw(Seiscomp::Core::ValueException);


	// ------------------------------------------------------------------
	//  Index management
	// ------------------------------------------------------------------
	public:
		//! Returns the object's index
		const ResponsePolynomialIndex& index() const;

		//! Checks two objects for equality regarding their index
		bool equalIndex(const ResponsePolynomial* lhs) const;

	
	// ------------------------------------------------------------------
	//  Public interface
	// ------------------------------------------------------------------
	public:
		Inventory* inventory() const;

		//! Implement Object interface
		bool assign(Object* other);
		bool attachTo(PublicObject* parent);
		bool detachFrom(PublicObject* parent);
		bool detach();

		//! Creates a clone
		Object* clone() const;

		//! Implement PublicObject interface
		bool updateChild(Object* child);

		void accept(Visitor*);


	// ------------------------------------------------------------------
	//  Implementation
	// ------------------------------------------------------------------
	private:
		// Index
		ResponsePolynomialIndex _index;

		// Attributes
		OPT(double) _gain;
		OPT(double) _gainFrequency;
		std::string _frequencyUnit;
		std::string _approximationType;
		OPT(double) _approximationLowerBound;
		OPT(double) _approximationUpperBound;
		OPT(double) _approximationError;
		OPT(int) _numberOfCoefficients;
		OPT(RealArray) _coefficients;
		OPT(Blob) _remark;

	DECLARE_SC_CLASSFACTORY_FRIEND(ResponsePolynomial);
};


}
}


#endif
