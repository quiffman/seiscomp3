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


#ifndef __SEISCOMP_DATAMODEL_AMPLITUDE_H__
#define __SEISCOMP_DATAMODEL_AMPLITUDE_H__


#include <seiscomp3/datamodel/creationinfo.h>
#include <seiscomp3/datamodel/timewindow.h>
#include <string>
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


DEFINE_SMARTPOINTER(Amplitude);
DEFINE_SMARTPOINTER(Comment);

class EventParameters;


class SC_CORE_DATAMODEL_API Amplitude : public PublicObject {
	DECLARE_SC_CLASS(Amplitude);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	protected:
		//! Protected constructor
		Amplitude();

	public:
		//! Copy constructor
		Amplitude(const Amplitude& other);

		//! Constructor with publicID
		Amplitude(const std::string& publicID);

		//! Destructor
		~Amplitude();
	

	// ------------------------------------------------------------------
	//  Creators
	// ------------------------------------------------------------------
	public:
		static Amplitude* Create();
		static Amplitude* Create(const std::string& publicID);


	// ------------------------------------------------------------------
	//  Lookup
	// ------------------------------------------------------------------
	public:
		static Amplitude* Find(const std::string& publicID);


	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		//! No changes regarding child objects are made
		Amplitude& operator=(const Amplitude& other);


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setType(const std::string& type);
		const std::string& type() const;

		void setAmplitude(const OPT(RealQuantity)& amplitude);
		RealQuantity& amplitude() throw(Seiscomp::Core::ValueException);
		const RealQuantity& amplitude() const throw(Seiscomp::Core::ValueException);

		void setTimeWindow(const OPT(TimeWindow)& timeWindow);
		TimeWindow& timeWindow() throw(Seiscomp::Core::ValueException);
		const TimeWindow& timeWindow() const throw(Seiscomp::Core::ValueException);

		void setPeriod(const OPT(RealQuantity)& period);
		RealQuantity& period() throw(Seiscomp::Core::ValueException);
		const RealQuantity& period() const throw(Seiscomp::Core::ValueException);

		void setSnr(const OPT(double)& snr);
		double snr() const throw(Seiscomp::Core::ValueException);

		void setPickID(const std::string& pickID);
		const std::string& pickID() const;

		void setWaveformID(const OPT(WaveformStreamID)& waveformID);
		WaveformStreamID& waveformID() throw(Seiscomp::Core::ValueException);
		const WaveformStreamID& waveformID() const throw(Seiscomp::Core::ValueException);

		void setFilterID(const std::string& filterID);
		const std::string& filterID() const;

		void setMethodID(const std::string& methodID);
		const std::string& methodID() const;

		void setScalingTime(const OPT(TimeQuantity)& scalingTime);
		TimeQuantity& scalingTime() throw(Seiscomp::Core::ValueException);
		const TimeQuantity& scalingTime() const throw(Seiscomp::Core::ValueException);

		void setMagnitudeHint(const std::string& magnitudeHint);
		const std::string& magnitudeHint() const;

		void setEvaluationMode(const OPT(EvaluationMode)& evaluationMode);
		EvaluationMode evaluationMode() const throw(Seiscomp::Core::ValueException);

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
		std::string _type;
		OPT(RealQuantity) _amplitude;
		OPT(TimeWindow) _timeWindow;
		OPT(RealQuantity) _period;
		OPT(double) _snr;
		std::string _pickID;
		OPT(WaveformStreamID) _waveformID;
		std::string _filterID;
		std::string _methodID;
		OPT(TimeQuantity) _scalingTime;
		std::string _magnitudeHint;
		OPT(EvaluationMode) _evaluationMode;
		OPT(CreationInfo) _creationInfo;

		// Aggregations
		std::vector<CommentPtr> _comments;

	DECLARE_SC_CLASSFACTORY_FRIEND(Amplitude);
};


}
}


#endif
