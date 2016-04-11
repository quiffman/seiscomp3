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


#ifndef __SEISCOMP_DATAMODEL_EVENTDESCRIPTION_H__
#define __SEISCOMP_DATAMODEL_EVENTDESCRIPTION_H__


#include <string>
#include <seiscomp3/datamodel/types.h>
#include <seiscomp3/datamodel/object.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(EventDescription);

class Event;


class SC_CORE_DATAMODEL_API EventDescriptionIndex {
	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		EventDescriptionIndex();
		EventDescriptionIndex(EventDescriptionType type);

		//! Copy constructor
		EventDescriptionIndex(const EventDescriptionIndex&);


	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		bool operator==(const EventDescriptionIndex&) const;
		bool operator!=(const EventDescriptionIndex&) const;


	// ------------------------------------------------------------------
	//  Attributes
	// ------------------------------------------------------------------
	public:
		EventDescriptionType type;
};


class SC_CORE_DATAMODEL_API EventDescription : public Object {
	DECLARE_SC_CLASS(EventDescription);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		EventDescription();

		//! Copy constructor
		EventDescription(const EventDescription& other);

		//! Custom constructor
		EventDescription(const std::string& text);
		EventDescription(const std::string& text,
		                 EventDescriptionType type);

		//! Destructor
		~EventDescription();
	

	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		EventDescription& operator=(const EventDescription& other);


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setText(const std::string& text);
		const std::string& text() const;

		void setType(EventDescriptionType type);
		EventDescriptionType type() const;


	// ------------------------------------------------------------------
	//  Index management
	// ------------------------------------------------------------------
	public:
		//! Returns the object's index
		const EventDescriptionIndex& index() const;

		//! Checks two objects for equality regarding their index
		bool equalIndex(const EventDescription* lhs) const;

	
	// ------------------------------------------------------------------
	//  Public interface
	// ------------------------------------------------------------------
	public:
		Event* event() const;

		//! Implement Object interface
		bool assign(Object* other);
		bool attachTo(PublicObject* parent);
		bool detachFrom(PublicObject* parent);
		bool detach();

		//! Creates a clone
		Object* clone() const;

		void accept(Visitor*);


	// ------------------------------------------------------------------
	//  Implementation
	// ------------------------------------------------------------------
	private:
		// Index
		EventDescriptionIndex _index;

		// Attributes
		std::string _text;
};


}
}


#endif
