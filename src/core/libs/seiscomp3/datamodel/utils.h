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


#ifndef __SEISCOMP_DATAMODEL_UTIL_H__
#define __SEISCOMP_DATAMODEL_UTIL_H__

#include <seiscomp3/core/datetime.h>
#include <seiscomp3/core/exceptions.h>
#include <seiscomp3/datamodel/api.h>
#include <seiscomp3/datamodel/notifier.h>
#include <seiscomp3/datamodel/object.h>
#include <seiscomp3/datamodel/types.h>
#include <string>
#include <vector>


namespace Seiscomp {
namespace DataModel {

class Event;
class Origin;
class Pick;
class Inventory;
class Station;
class SensorLocation;
class Stream;


SC_CORE_DATAMODEL_API std::string eventRegion(const DataModel::Event *);


template <typename T>
char objectStatusToChar(const T *o) {
	EvaluationMode mode = AUTOMATIC;

	try { mode = o->evaluationMode(); } catch ( ... ) {}

	// Manual origin are always tagged as M
	if ( mode == MANUAL ) return 'M';

	try {
		switch ( o->evaluationStatus() ) {
			case PRELIMINARY:
				return 'P';
			case CONFIRMED:
			case REVIEWED:
			case FINAL:
				return 'C';
			case REJECTED:
				return 'X';
			case REPORTED:
				return 'R';
			default:
				break;
		}
	}
	catch ( ... ) {}

	return 'A';
}


template <typename T>
std::string objectAgencyID(const T *o) {
	try {
		return o->creationInfo().agencyID();
	}
	catch ( Core::ValueException & ) {}

	return "";
}


template <typename T>
std::string objectAuthor(const T *o) {
	try {
		return o->creationInfo().author();
	}
	catch ( Core::ValueException & ) {}

	return "";
}


template <typename T>
double quantityUncertainty(const T &o) {
	try {
		return o.uncertainty();
	}
	catch ( Core::ValueException & ) {
		return (o.lowerUncertainty() + o.upperUncertainty()) * 0.5;
	}
}


//! Returns the station for a network- and stationcode and
//! a time. If the station has not been found NULL will be returned.
SC_CORE_DATAMODEL_API
Station* getStation(const Inventory *inventory,
                    const std::string &networkCode,
                    const std::string &stationCode,
                    const Core::Time &);

//! Returns the sensorlocation for a network-, station- and locationcode and
//! a time. If the sensorlocation has not been found NULL will be returned.
SC_CORE_DATAMODEL_API
SensorLocation* getSensorLocation(const Inventory *inventory,
                                  const std::string &networkCode,
                                  const std::string &stationCode,
                                  const std::string &locationCode,
                                  const Core::Time &);

//! Returns the stream for a network-, station-, location- and channelcode and
//! a time. If the stream has not been found NULL will be returned.
SC_CORE_DATAMODEL_API
Stream* getStream(const Inventory *inventory,
                  const std::string &networkCode,
                  const std::string &stationCode,
                  const std::string &locationCode,
                  const std::string &channelCode,
                  const Core::Time &);

//! Returns the station used for a pick. If the station has not been found
//! NULL will be returned.
SC_CORE_DATAMODEL_API
Station* getStation(const Inventory *inventory, const Pick *pick);


//! Returns the sensor location used for a pick. If the sensor location has
//! not been found NULL will be returned.
SC_CORE_DATAMODEL_API
SensorLocation* getSensorLocation(const Inventory *inventory,
                                  const Pick *pick);


//! Returns the stream used for a pick. If the stream has
//! not been found NULL will be returned.
Stream* getStream(const Inventory *inventory,
                  const Pick *pick);


struct ThreeComponents {
	enum Component {
		Vertical         = 0,  /* usually Z */
		FirstHorizontal  = 1,  /* usually N */
		SecondHorizontal = 2   /* usually E */
	};

	ThreeComponents();

	Stream  *comps[3];
};


//! Returns the best matching stream for the vertical component having
//! the stream code set to streamCode (without component code, first two letters).
SC_CORE_DATAMODEL_API Stream *
getVerticalComponent(const SensorLocation *loc, const char *streamCode, const Core::Time &time);

//! Returns the best matching streams for the vertical and horizontal components
//! having the stream code set to streamCode (without component code, first two letters).
SC_CORE_DATAMODEL_API void
getThreeComponents(ThreeComponents &res, const SensorLocation *loc, const char *streamCode, const Core::Time &time);


/**
 * Creates a deep copy of an object including all child objects.
 */
SC_CORE_DATAMODEL_API Object *copy(Object* obj);


///////////////////////////////////////////////////////////////////////////////
// DataModel Diff
///////////////////////////////////////////////////////////////////////////////

/**
 * Scans a object tree for a particular node. Objects are compared on base of
 * their indexes, @see equalsIndex
 * @param tree object tree to scan
 * @param node item to search for
 * @return pointer to the item within the object tree or NULL if the node was
 * not found
 */
SC_CORE_DATAMODEL_API Object*
find(Object* tree, Object* node);

/**
 * Recursively compares two objects and collects all differences.
 * The root element of one of the objects must be included in the other object
 * tree, @see find(o1, o2) 
 * @param o1 first object to compare
 * @param o2 second object to compare
 * @param diffList list to collect differences in
 * @return true if the diff could be performed, false if one object was null or
 * no common child object could be found
 * @throw TypeException if any type restriction is violated
 */
SC_CORE_DATAMODEL_API bool
diff(Object* o1, Object* o2, std::vector<NotifierPtr>& diffList);

/**
 * Recursively merges the node object (and its children) into the tree object.
 * The node must be part of the tree, @ref find(o1, o2). Properties of the node
 * object override properties of the tree.
 * @param tree main object to merge the node into
 * @param node object to be merged into the tree
 * @param idMap map that keeps track of any publicID attribute changes applied
 * during the merge
 * @return true if the merge could be performed, false if the node was not found
 * in the tree
 * @throw TypeException if any type restriction is violated
 */
SC_CORE_DATAMODEL_API bool
merge(Object* tree, Object* node, std::map<std::string, std::string>& idMap);

/**
 * Merges all all objects in the vector in order of their appearance into the
 * mergeResult object, @ref merge(Object*, Object*). The mergeResult object must
 * be not null and must serve as a parent for the objects being merged. In a
 * subsequent processing step changes to publicIDs are applied to references,
 * @ref mapReferences.
 * @param mergeResult object to merge the vector into
 * @param objects vector of objects to merge
 * @return true if all objects could be merged successfully, else false.
 */
SC_CORE_DATAMODEL_API bool
merge(Object* mergeResult, const std::vector<Object*>& objects);

/**
 * Validates the internal publicID references of the specified object. In a
 * first step all publicIDs are collected, then the object is traversed top-down
 * and each reference's value is searched in the publicID set.
 * @param o object to validate
 * @return true if all references point to an existing publicID, else false
 */
SC_CORE_DATAMODEL_API bool
validateReferences(Object* o);

/**
 * Maps publicID references of the specified object. While the object is
 * traversed top-down, a lookup for each reference in the publicID map is
 * performed. If a matching entry is found the reference's value is updated.
 * @param o object which references should be mapped
 * @param map publicIDMap of deprecated to current publicIDs
 * @return number of mappings performed
 */
SC_CORE_DATAMODEL_API size_t
mapReferences(Object* o, const std::map<std::string, std::string> &publicIDMap);


} // of ns DataModel
} // of ns Seiscomp


#endif
