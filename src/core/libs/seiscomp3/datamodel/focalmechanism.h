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


#ifndef __SEISCOMP_DATAMODEL_FOCALMECHANISM_H__
#define __SEISCOMP_DATAMODEL_FOCALMECHANISM_H__


#include <seiscomp3/datamodel/creationinfo.h>
#include <string>
#include <seiscomp3/datamodel/types.h>
#include <seiscomp3/datamodel/nodalplanes.h>
#include <vector>
#include <seiscomp3/datamodel/principalaxes.h>
#include <seiscomp3/datamodel/comment.h>
#include <seiscomp3/datamodel/notifier.h>
#include <seiscomp3/datamodel/publicobject.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(FocalMechanism);
DEFINE_SMARTPOINTER(Comment);
DEFINE_SMARTPOINTER(MomentTensor);

class EventParameters;


class SC_CORE_DATAMODEL_API FocalMechanism : public PublicObject {
	DECLARE_SC_CLASS(FocalMechanism);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	protected:
		//! Protected constructor
		FocalMechanism();

	public:
		//! Copy constructor
		FocalMechanism(const FocalMechanism& other);

		//! Constructor with publicID
		FocalMechanism(const std::string& publicID);

		//! Destructor
		~FocalMechanism();
	

	// ------------------------------------------------------------------
	//  Creators
	// ------------------------------------------------------------------
	public:
		static FocalMechanism* Create();
		static FocalMechanism* Create(const std::string& publicID);


	// ------------------------------------------------------------------
	//  Lookup
	// ------------------------------------------------------------------
	public:
		static FocalMechanism* Find(const std::string& publicID);


	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		//! No changes regarding child objects are made
		FocalMechanism& operator=(const FocalMechanism& other);


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setTriggeringOriginID(const std::string& triggeringOriginID);
		const std::string& triggeringOriginID() const;

		void setNodalPlanes(const OPT(NodalPlanes)& nodalPlanes);
		NodalPlanes& nodalPlanes() throw(Seiscomp::Core::ValueException);
		const NodalPlanes& nodalPlanes() const throw(Seiscomp::Core::ValueException);

		void setPrincipalAxes(const OPT(PrincipalAxes)& principalAxes);
		PrincipalAxes& principalAxes() throw(Seiscomp::Core::ValueException);
		const PrincipalAxes& principalAxes() const throw(Seiscomp::Core::ValueException);

		void setAzimuthalGap(const OPT(double)& azimuthalGap);
		double azimuthalGap() const throw(Seiscomp::Core::ValueException);

		void setStationPolarityCount(const OPT(int)& stationPolarityCount);
		int stationPolarityCount() const throw(Seiscomp::Core::ValueException);

		void setMisfit(const OPT(double)& misfit);
		double misfit() const throw(Seiscomp::Core::ValueException);

		void setStationDistributionRatio(const OPT(double)& stationDistributionRatio);
		double stationDistributionRatio() const throw(Seiscomp::Core::ValueException);

		void setMethodID(const std::string& methodID);
		const std::string& methodID() const;

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
		bool add(MomentTensor* obj);

		/**
		 * Removes an object.
		 * @param obj The object pointer
		 * @return true The object has been removed
		 * @return false The object has not been removed
		 *               because it does not exist in the list
		 */
		bool remove(Comment* obj);
		bool remove(MomentTensor* obj);

		/**
		 * Removes an object of a particular class.
		 * @param i The object index
		 * @return true The object has been removed
		 * @return false The index is out of bounds
		 */
		bool removeComment(size_t i);
		bool removeComment(const CommentIndex& i);
		bool removeMomentTensor(size_t i);

		//! Retrieve the number of objects of a particular class
		size_t commentCount() const;
		size_t momentTensorCount() const;

		//! Index access
		//! @return The object at index i
		Comment* comment(size_t i) const;
		Comment* comment(const CommentIndex& i) const;
		MomentTensor* momentTensor(size_t i) const;

		//! Find an object by its unique attribute(s)
		MomentTensor* findMomentTensor(const std::string& publicID) const;

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
		std::string _triggeringOriginID;
		OPT(NodalPlanes) _nodalPlanes;
		OPT(PrincipalAxes) _principalAxes;
		OPT(double) _azimuthalGap;
		OPT(int) _stationPolarityCount;
		OPT(double) _misfit;
		OPT(double) _stationDistributionRatio;
		std::string _methodID;
		OPT(EvaluationMode) _evaluationMode;
		OPT(EvaluationStatus) _evaluationStatus;
		OPT(CreationInfo) _creationInfo;

		// Aggregations
		std::vector<CommentPtr> _comments;
		std::vector<MomentTensorPtr> _momentTensors;

	DECLARE_SC_CLASSFACTORY_FRIEND(FocalMechanism);
};


}
}


#endif
