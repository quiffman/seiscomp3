/***************************************************************************
 *   Copyright (C) 2009 by gempa GmbH
 *
 *   Author: Jan Becker
 *   Email: jabe@gempa.de $
 *
 ***************************************************************************/


#ifndef __SEISCOMP_STATIONXML_STAMESSAGE_H__
#define __SEISCOMP_STATIONXML_STAMESSAGE_H__


#include <stationxml/metadata.h>
#include <vector>
#include <string>
#include <stationxml/date.h>
#include <seiscomp3/core/baseobject.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace StationXML {

DEFINE_SMARTPOINTER(Network);
DEFINE_SMARTPOINTER(Station);



DEFINE_SMARTPOINTER(StaMessage);


/**
 * \brief Top-level type for Station XML. Required field are Source (network
 * \brief ID of the institution sending the message) and one or more Network
 * \brief containers or one or more Station containers.
 */
class StaMessage : public Core::BaseObject {
	DECLARE_CASTS(StaMessage);
	DECLARE_RTTI;
	DECLARE_METAOBJECT_DERIVED;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		StaMessage();

		//! Copy constructor
		StaMessage(const StaMessage& other);

		//! Destructor
		~StaMessage();


	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		StaMessage& operator=(const StaMessage& other);
		bool operator==(const StaMessage& other) const;


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		//! Network ID of the institution sending the message.
		//! XML tag: Source
		void setSource(const std::string& source);
		const std::string& source() const;

		//! Name of the institution sending this message.
		//! XML tag: Sender
		void setSender(const std::string& sender);
		const std::string& sender() const;

		//! Name of the software module that generated this document.
		//! XML tag: Module
		void setModule(const std::string& module);
		const std::string& module() const;

		//! This is the address of the query that generated the StationXML
		//! document, or, if applicable, the address of the software that
		//! generated this document.
		//! XML tag: ModuleURI
		void setModuleURI(const std::string& moduleURI);
		const std::string& moduleURI() const;

		//! XML tag: SentDate
		void setSentDate(DateTime sentDate);
		DateTime sentDate() const;

	
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
		bool addNetwork(Network* obj);
		bool addStation(Station* obj);

		/**
		 * Removes an object.
		 * @param obj The object pointer
		 * @return true The object has been removed
		 * @return false The object has not been removed
		 *               because it does not exist in the list
		 */
		bool removeNetwork(Network* obj);
		bool removeStation(Station* obj);

		/**
		 * Removes an object of a particular class.
		 * @param i The object index
		 * @return true The object has been removed
		 * @return false The index is out of bounds
		 */
		bool removeNetwork(size_t i);
		bool removeStation(size_t i);

		//! Retrieve the number of objects of a particular class
		size_t networkCount() const;
		size_t stationCount() const;

		//! Index access
		//! @return The object at index i
		Network* network(size_t i) const;
		Station* station(size_t i) const;


	// ------------------------------------------------------------------
	//  Implementation
	// ------------------------------------------------------------------
	private:
		// Attributes
		std::string _source;
		std::string _sender;
		std::string _module;
		std::string _moduleURI;
		DateTime _sentDate;

		// Aggregations
		std::vector<NetworkPtr> _networks;
		std::vector<StationPtr> _stations;
};


}
}


#endif