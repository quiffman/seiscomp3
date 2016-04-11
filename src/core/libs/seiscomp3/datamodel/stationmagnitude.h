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


#ifndef __SEISCOMP_DATAMODEL_STATIONMAGNITUDE_H__
#define __SEISCOMP_DATAMODEL_STATIONMAGNITUDE_H__


#include <seiscomp3/datamodel/creationinfo.h>
#include <vector>
#include <string>
#include <seiscomp3/datamodel/realquantity.h>
#include <seiscomp3/datamodel/waveformstreamid.h>
#include <seiscomp3/datamodel/comment.h>
#include <seiscomp3/datamodel/notifier.h>
#include <seiscomp3/datamodel/publicobject.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(StationMagnitude);
DEFINE_SMARTPOINTER(Comment);

class Origin;


class SC_CORE_DATAMODEL_API StationMagnitude : public PublicObject {
	DECLARE_SC_CLASS(StationMagnitude);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	protected:
		//! Protected constructor
		StationMagnitude();

	public:
		//! Copy constructor
		StationMagnitude(const StationMagnitude& other);

		//! Constructor with publicID
		StationMagnitude(const std::string& publicID);

		//! Destructor
		~StationMagnitude();
	

	// ------------------------------------------------------------------
	//  Creators
	// ------------------------------------------------------------------
	public:
		static StationMagnitude* Create();
		static StationMagnitude* Create(const std::string& publicID);


	// ------------------------------------------------------------------
	//  Lookup
	// ------------------------------------------------------------------
	public:
		static StationMagnitude* Find(const std::string& publicID);


	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		//! No changes regarding child objects are made
		StationMagnitude& operator=(const StationMagnitude& other);


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setOriginID(const std::string& originID);
		const std::string& originID() const;

		void setMagnitude(const RealQuantity& magnitude);
		RealQuantity& magnitude();
		const RealQuantity& magnitude() const;

		void setType(const std::string& type);
		const std::string& type() const;

		void setAmplitudeID(const std::string& amplitudeID);
		const std::string& amplitudeID() const;

		void setMethodID(const std::string& methodID);
		const std::string& methodID() const;

		void setWaveformID(const OPT(WaveformStreamID)& waveformID);
		WaveformStreamID& waveformID() throw(Seiscomp::Core::ValueException);
		const WaveformStreamID& waveformID() const throw(Seiscomp::Core::ValueException);

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

		Origin* origin() const;

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
		std::string _originID;
		RealQuantity _magnitude;
		std::string _type;
		std::string _amplitudeID;
		std::string _methodID;
		OPT(WaveformStreamID) _waveformID;
		OPT(CreationInfo) _creationInfo;

		// Aggregations
		std::vector<CommentPtr> _comments;

	DECLARE_SC_CLASSFACTORY_FRIEND(StationMagnitude);
};


}
}


#endif