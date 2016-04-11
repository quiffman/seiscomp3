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


#ifndef __SEISCOMP_DATAMODEL_SOURCETIMEFUNCTION_H__
#define __SEISCOMP_DATAMODEL_SOURCETIMEFUNCTION_H__


#include <seiscomp3/datamodel/types.h>
#include <seiscomp3/core/baseobject.h>
#include <seiscomp3/datamodel/api.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(SourceTimeFunction);


class SC_CORE_DATAMODEL_API SourceTimeFunction : public Core::BaseObject {
	DECLARE_SC_CLASS(SourceTimeFunction);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		SourceTimeFunction();

		//! Copy constructor
		SourceTimeFunction(const SourceTimeFunction& other);

		//! Destructor
		~SourceTimeFunction();
	

	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		SourceTimeFunction& operator=(const SourceTimeFunction& other);
		bool operator==(const SourceTimeFunction& other) const;
		bool operator!=(const SourceTimeFunction& other) const;


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setType(SourceTimeFunctionType type);
		SourceTimeFunctionType type() const;

		void setDuration(double duration);
		double duration() const;

		void setRiseTime(const OPT(double)& riseTime);
		double riseTime() const throw(Seiscomp::Core::ValueException);

		void setDecayTime(const OPT(double)& decayTime);
		double decayTime() const throw(Seiscomp::Core::ValueException);


	// ------------------------------------------------------------------
	//  Implementation
	// ------------------------------------------------------------------
	private:
		// Attributes
		SourceTimeFunctionType _type;
		double _duration;
		OPT(double) _riseTime;
		OPT(double) _decayTime;
};


}
}


#endif
