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


#ifndef __SEISCOMP_DATAMODEL_RESPONSEFIR_H__
#define __SEISCOMP_DATAMODEL_RESPONSEFIR_H__


#include <seiscomp3/datamodel/blob.h>
#include <string>
#include <seiscomp3/datamodel/realarray.h>
#include <seiscomp3/datamodel/publicobject.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(ResponseFIR);

class Inventory;


class SC_CORE_DATAMODEL_API ResponseFIRIndex {
	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		ResponseFIRIndex();
		ResponseFIRIndex(const std::string& name);

		//! Copy constructor
		ResponseFIRIndex(const ResponseFIRIndex&);


	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		bool operator==(const ResponseFIRIndex&) const;
		bool operator!=(const ResponseFIRIndex&) const;


	// ------------------------------------------------------------------
	//  Attributes
	// ------------------------------------------------------------------
	public:
		std::string name;
};


class SC_CORE_DATAMODEL_API ResponseFIR : public PublicObject {
	DECLARE_SC_CLASS(ResponseFIR);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	protected:
		//! Protected constructor
		ResponseFIR();

	public:
		//! Copy constructor
		ResponseFIR(const ResponseFIR& other);

		//! Constructor with publicID
		ResponseFIR(const std::string& publicID);

		//! Destructor
		~ResponseFIR();
	

	// ------------------------------------------------------------------
	//  Creators
	// ------------------------------------------------------------------
	public:
		static ResponseFIR* Create();
		static ResponseFIR* Create(const std::string& publicID);


	// ------------------------------------------------------------------
	//  Lookup
	// ------------------------------------------------------------------
	public:
		static ResponseFIR* Find(const std::string& publicID);


	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		//! No changes regarding child objects are made
		ResponseFIR& operator=(const ResponseFIR& other);
		//! Checks for equality of two objects. Childs objects
		//! are not part of the check.
		bool operator==(const ResponseFIR& other) const;
		bool operator!=(const ResponseFIR& other) const;

		//! Wrapper that calls operator==
		bool equal(const ResponseFIR& other) const;


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setName(const std::string& name);
		const std::string& name() const;

		void setGain(const OPT(double)& gain);
		double gain() const throw(Seiscomp::Core::ValueException);

		void setDecimationFactor(const OPT(int)& decimationFactor);
		int decimationFactor() const throw(Seiscomp::Core::ValueException);

		void setDelay(const OPT(double)& delay);
		double delay() const throw(Seiscomp::Core::ValueException);

		void setCorrection(const OPT(double)& correction);
		double correction() const throw(Seiscomp::Core::ValueException);

		void setNumberOfCoefficients(const OPT(int)& numberOfCoefficients);
		int numberOfCoefficients() const throw(Seiscomp::Core::ValueException);

		void setSymmetry(const std::string& symmetry);
		const std::string& symmetry() const;

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
		const ResponseFIRIndex& index() const;

		//! Checks two objects for equality regarding their index
		bool equalIndex(const ResponseFIR* lhs) const;

	
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
		ResponseFIRIndex _index;

		// Attributes
		OPT(double) _gain;
		OPT(int) _decimationFactor;
		OPT(double) _delay;
		OPT(double) _correction;
		OPT(int) _numberOfCoefficients;
		std::string _symmetry;
		OPT(RealArray) _coefficients;
		OPT(Blob) _remark;

	DECLARE_SC_CLASSFACTORY_FRIEND(ResponseFIR);
};


}
}


#endif
