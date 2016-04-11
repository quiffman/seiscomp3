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


#ifndef __SEISCOMP_DATAMODEL_TIMEWINDOW_H__
#define __SEISCOMP_DATAMODEL_TIMEWINDOW_H__


#include <seiscomp3/core/datetime.h>
#include <seiscomp3/core/baseobject.h>
#include <seiscomp3/datamodel/api.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(TimeWindow);


class SC_CORE_DATAMODEL_API TimeWindow : public Core::BaseObject {
	DECLARE_SC_CLASS(TimeWindow);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		TimeWindow();

		//! Copy constructor
		TimeWindow(const TimeWindow& other);

		//! Custom constructor
		TimeWindow(Seiscomp::Core::Time reference);
		TimeWindow(Seiscomp::Core::Time reference,
		           double begin,
		           double end);

		//! Destructor
		~TimeWindow();
	

	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		operator Seiscomp::Core::Time&();
		operator Seiscomp::Core::Time() const;

		//! Copies the metadata of other to this
		TimeWindow& operator=(const TimeWindow& other);
		bool operator==(const TimeWindow& other) const;
		bool operator!=(const TimeWindow& other) const;


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setReference(Seiscomp::Core::Time reference);
		Seiscomp::Core::Time reference() const;

		void setBegin(double begin);
		double begin() const;

		void setEnd(double end);
		double end() const;


	// ------------------------------------------------------------------
	//  Implementation
	// ------------------------------------------------------------------
	private:
		// Attributes
		Seiscomp::Core::Time _reference;
		double _begin;
		double _end;
};


}
}


#endif