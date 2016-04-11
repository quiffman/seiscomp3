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


#ifndef __SEISCOMP_DATAMODEL_PICK_H__
#define __SEISCOMP_DATAMODEL_PICK_H__


#include <seiscomp3/datamodel/creationinfo.h>
#include <string>
#include <seiscomp3/datamodel/phase.h>
#include <seiscomp3/datamodel/types.h>
#include <seiscomp3/datamodel/waveformstreamid.h>
#include <vector>
#include <seiscomp3/datamodel/timequantity.h>
#include <seiscomp3/datamodel/realquantity.h>
#include <seiscomp3/datamodel/comment.h>
#include <seiscomp3/datamodel/notifier.h>
#include <seiscomp3/datamodel/publicobject.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(Pick);
DEFINE_SMARTPOINTER(Comment);

class EventParameters;


class SC_CORE_DATAMODEL_API Pick : public PublicObject {
	DECLARE_SC_CLASS(Pick);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	protected:
		//! Protected constructor
		Pick();

	public:
		//! Copy constructor
		Pick(const Pick& other);

		//! Constructor with publicID
		Pick(const std::string& publicID);

		//! Destructor
		~Pick();
	

	// ------------------------------------------------------------------
	//  Creators
	// ------------------------------------------------------------------
	public:
		static Pick* Create();
		static Pick* Create(const std::string& publicID);


	// ------------------------------------------------------------------
	//  Lookup
	// ------------------------------------------------------------------
	public:
		static Pick* Find(const std::string& publicID);


	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		//! No changes regarding child objects are made
		Pick& operator=(const Pick& other);


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setTime(const TimeQuantity& time);
		TimeQuantity& time();
		const TimeQuantity& time() const;

		void setWaveformID(const WaveformStreamID& waveformID);
		WaveformStreamID& waveformID();
		const WaveformStreamID& waveformID() const;

		void setFilterID(const std::string& filterID);
		const std::string& filterID() const;

		void setMethodID(const std::string& methodID);
		const std::string& methodID() const;

		void setHorizontalSlowness(const OPT(RealQuantity)& horizontalSlowness);
		RealQuantity& horizontalSlowness() throw(Seiscomp::Core::ValueException);
		const RealQuantity& horizontalSlowness() const throw(Seiscomp::Core::ValueException);

		void setBackazimuth(const OPT(RealQuantity)& backazimuth);
		RealQuantity& backazimuth() throw(Seiscomp::Core::ValueException);
		const RealQuantity& backazimuth() const throw(Seiscomp::Core::ValueException);

		void setSlownessMethodID(const std::string& slownessMethodID);
		const std::string& slownessMethodID() const;

		void setOnset(const OPT(PickOnset)& onset);
		PickOnset onset() const throw(Seiscomp::Core::ValueException);

		void setPhaseHint(const OPT(Phase)& phaseHint);
		Phase& phaseHint() throw(Seiscomp::Core::ValueException);
		const Phase& phaseHint() const throw(Seiscomp::Core::ValueException);

		void setPolarity(const OPT(PickPolarity)& polarity);
		PickPolarity polarity() const throw(Seiscomp::Core::ValueException);

		void setEvaluationMode(const OPT(EvaluationMode)& evaluationMode);
		EvaluationMode evaluationMode() const throw(Seiscomp::Core::ValueException);

		void setEvaluationStatus(const OPT(EvaluationStatus)& evaluationStatus);
		EvaluationStatus evaluationStatus() const throw(Seiscomp::Core::ValueException);

		void setCreationInfo(const OPT(CreationInfo)& creationInfo);
		CreationInfo& creationInfo() throw(Seiscomp::Core::ValueException);
		const CreationInfo& creationInfo() const throw(Seiscomp::Core::ValueException);

	
	// ------------------------------------------------------------------
	//  Public interface
	// ------------------------------------------------------------------
	public:
		/**
		 * Add an object.
		 * @param obj The object pointer
		 * @return true The object has been added
		 * @return false The object has not been added
		 *               because it already exists in the list
		 *               or it already has another parent
		 */
		bool add(Comment* obj);

		/**
		 * Removes an object.
		 * @param obj The object pointer
		 * @return true The object has been removed
		 * @return false The object has not been removed
		 *               because it does not exist in the list
		 */
		bool remove(Comment* obj);

		/**
		 * Removes an object of a particular class.
		 * @param i The object index
		 * @return true The object has been removed
		 * @return false The index is out of bounds
		 */
		bool removeComment(size_t i);
		bool removeComment(const CommentIndex& i);

		//! Retrieve the number of objects of a particular class
		size_t commentCount() const;

		//! Index access
		//! @return The object at index i
		Comment* comment(size_t i) const;
		Comment* comment(const CommentIndex& i) const;

		//! Find an object by its unique attribute(s)

		EventParameters* eventParameters() const;

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
		// Attributes
		TimeQuantity _time;
		WaveformStreamID _waveformID;
		std::string _filterID;
		std::string _methodID;
		OPT(RealQuantity) _horizontalSlowness;
		OPT(RealQuantity) _backazimuth;
		std::string _slownessMethodID;
		OPT(PickOnset) _onset;
		OPT(Phase) _phaseHint;
		OPT(PickPolarity) _polarity;
		OPT(EvaluationMode) _evaluationMode;
		OPT(EvaluationStatus) _evaluationStatus;
		OPT(CreationInfo) _creationInfo;

		// Aggregations
		std::vector<CommentPtr> _comments;

	DECLARE_SC_CLASSFACTORY_FRIEND(Pick);
};


}
}


#endif
