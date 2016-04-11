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
#define SEISCOMP_COMPONENT DataModelUtils

#include <seiscomp3/core/exceptions.h>
#include <seiscomp3/core/metaobject.h>
#include <seiscomp3/datamodel/event.h>
#include <seiscomp3/datamodel/eventdescription.h>
#include <seiscomp3/datamodel/inventory.h>
#include <seiscomp3/datamodel/origin.h>
#include <seiscomp3/datamodel/pick.h>
#include <seiscomp3/datamodel/station.h>
#include <seiscomp3/datamodel/sensorlocation.h>
#include <seiscomp3/datamodel/stream.h>
#include <seiscomp3/datamodel/utils.h>
#include <seiscomp3/logging/log.h>
#include <seiscomp3/math/math.h>

#include <cstring>
#include <deque>
#include <iostream>

namespace Seiscomp {
namespace DataModel {

namespace {

class CloneVisitor : public Visitor {
	public:
		CloneVisitor() : Visitor(TM_TOPDOWN) { reset(); }

		// Reset should be called before the object is used
		// a second time.
		void reset() {
			_clone = NULL;
			_parents.clear();
		}

		bool visit(PublicObject *po) {
			PublicObject *clone = static_cast<PublicObject*>(po->clone());
			if ( !commit(clone) ) return false;
			_parents.push_front(clone);
			return true;
		}

		void finished() {
			_parents.pop_front();
		}

		void visit(Object *o) {
			Object *clone = o->clone();
			commit(clone);
		}

		Object *result() const {
			return _clone;
		}

	private:
		bool commit(Object *o) {
			if ( !_parents.empty() ) {
				if ( !o->attachTo(_parents.front()) ) {
					delete o;
					o = NULL;
					return false;
				}
			}

			if ( _clone == NULL ) _clone = o;
			return true;
		}

	private:
		std::deque<PublicObject*> _parents;
		Object *_clone;
};

}


ThreeComponents::ThreeComponents() {
	comps[0] = NULL;
	comps[1] = NULL;
	comps[2] = NULL;
}


std::string eventRegion(const Event *ev) {
	for ( size_t i = 0; i < ev->eventDescriptionCount(); ++i ) {
		EventDescription *ed = ev->eventDescription(i);
		try {
			if ( ed->type() == REGION_NAME ) {
				return ed->text();
			}
		}
		catch ( ... ) {}
	}
	return "";
}


Station* getStation(const Inventory *inventory,
                    const std::string &networkCode,
                    const std::string &stationCode,
                    const Core::Time &time) {
	if ( inventory == NULL )
		return NULL;

	for ( size_t i = 0; i < inventory->networkCount(); ++i ) {
		DataModel::Network* network = inventory->network(i);
		if ( network->code() != networkCode ) continue;

		try {
			if ( network->end() < time ) continue;
		}
		catch (...) {
		}

		if ( network->start() > time ) continue;

		for ( size_t j = 0; j < network->stationCount(); ++j ) {
			DataModel::Station* station = network->station(j);
			if ( station->code() != stationCode ) continue;

			try {
				if ( station->end() < time ) continue;
			}
			catch (...) {}

			if ( station->start() > time ) continue;

			return station;
		}
	}

	return NULL;
}


SensorLocation* getSensorLocation(const Inventory *inventory,
                                  const std::string &networkCode,
                                  const std::string &stationCode,
                                  const std::string &locationCode,
                                  const Core::Time &time) {
	DataModel::Station *sta = getStation(inventory, networkCode, stationCode, time);
	if ( sta == NULL )
		return NULL;

	for ( size_t i = 0; i < sta->sensorLocationCount(); ++i ) {
		DataModel::SensorLocation* loc = sta->sensorLocation(i);
		if ( loc->code() != locationCode ) continue;

		try {
			if ( loc->end() <= time ) continue;
		}
		catch (...) {}

		if ( loc->start() > time ) continue;

		return loc;
	}

	return NULL;
}


Stream* getStream(const Inventory *inventory,
                  const std::string &networkCode,
                  const std::string &stationCode,
                  const std::string &locationCode,
                  const std::string &channelCode,
                  const Core::Time &time) {
	DataModel::SensorLocation *loc = getSensorLocation(inventory, networkCode, stationCode, locationCode, time);
	if ( loc == NULL )
		return NULL;

	for ( size_t i = 0; i < loc->streamCount(); ++i ) {
		DataModel::Stream *stream = loc->stream(i);
		if ( stream->code() != channelCode ) continue;

		try {
			if ( stream->end() <= time ) continue;
		}
		catch (...) {}

		if ( stream->start() > time ) continue;

		return stream;
	}

	return NULL;
}


Station* getStation(const Inventory *inventory, const Pick *pick) {
	if ( pick == NULL ) return NULL;

	return getStation(inventory, pick->waveformID().networkCode(),
	                  pick->waveformID().stationCode(),
	                  pick->time().value());
}


SensorLocation* getSensorLocation(const Inventory *inventory,
                                  const Pick *pick) {
	if ( pick == NULL ) return NULL;

	return getSensorLocation(inventory, pick->waveformID().networkCode(),
	                         pick->waveformID().stationCode(), pick->waveformID().locationCode(),
	                         pick->time().value());
}


Stream* getStream(const Inventory *inventory,
                  const Pick *pick) {
	if ( pick == NULL ) return NULL;

	return getStream(inventory, pick->waveformID().networkCode(),
	                 pick->waveformID().stationCode(), pick->waveformID().locationCode(),
	                 pick->waveformID().channelCode(), pick->time().value());
}


Stream *getVerticalComponent(const SensorLocation *loc, const char *streamCode, const Core::Time &time) {
	int len = strlen(streamCode);

	Stream *best = NULL;
	float maxCorr = -100;

	for ( size_t i = 0; i < loc->streamCount(); ++i ) {
		Stream *stream = loc->stream(i);

		try {
			if ( stream->end() < time ) continue;
		}
		catch (...) {}

		if ( stream->start() > time ) continue;

		if ( stream->code().compare(0, len, streamCode) )
			continue;

		try {
			// Z is up vector. Since we do not care about the sign
			// the maximum absolute value will be selected.
			float z = (float)fabs(sin(-deg2rad(stream->dip())));

			if ( z > maxCorr ) {
				maxCorr = z;
				best = stream;
			}
		}
		catch ( ... ) {}
	}

	return best;
}


void getThreeComponents(ThreeComponents &res, const SensorLocation *loc, const char *streamCode, const Core::Time &time) {
	int len = strlen(streamCode);

	res = ThreeComponents();

	float maxCorrZ = -100;
	float maxCorr1 = 100;
	float maxCorr2 = 100;

	float dirz[3], dir1[3], dir2[3];

	for ( size_t i = 0; i < loc->streamCount(); ++i ) {
		Stream *stream = loc->stream(i);

		try {
			if ( stream->end() < time ) continue;
		}
		catch (...) {}

		if ( stream->start() > time ) continue;

		if ( stream->code().compare(0, len, streamCode) )
			continue;

		try {
			float azi = (float)deg2rad(stream->azimuth());
			float dip = (float)deg2rad(stream->dip());

			// Store up vector
			float z = sin(-dip);

			float metric = fabs(z);

			if ( metric > maxCorrZ ) {
				maxCorrZ = metric;
				res.comps[ThreeComponents::Vertical] = stream;
				dirz[0] = cos(-dip)*sin(azi);
				dirz[1] = cos(-dip)*cos(azi);
				dirz[2] = z;
			}

			if ( metric < maxCorr1 ) {
				// Convert into normal space (X east, Y north, Z up)
				res.comps[ThreeComponents::FirstHorizontal] = stream;
				maxCorr1 = metric;
				dir1[0] = cos(-dip)*sin(azi);
				dir1[1] = cos(-dip)*cos(azi);
				dir1[2] = z;
			}
			else if ( metric < maxCorr2 ) {
				// Convert into normal space (X east, Y north, Z up)
				res.comps[ThreeComponents::SecondHorizontal] = stream;
				maxCorr2 = metric;
				dir2[0] = cos(-dip)*sin(azi);
				dir2[1] = cos(-dip)*cos(azi);
				dir2[2] = z;
			}
		}
		catch ( ... ) {}
	}

	if ( res.comps[ThreeComponents::Vertical] == res.comps[ThreeComponents::FirstHorizontal] ) {
		res.comps[ThreeComponents::FirstHorizontal] = res.comps[ThreeComponents::SecondHorizontal];
		res.comps[ThreeComponents::SecondHorizontal] = NULL;
		return;
	}

	// No vertical and only one or no horizontal found, return
	if ( !res.comps[ThreeComponents::Vertical] || !res.comps[ThreeComponents::SecondHorizontal] ) return;

	// Select the first horizontal left (math. positive) from the second
	float cross[3];
	cross[0] = dir1[1]*dir2[2] - dir1[2]*dir2[1];
	cross[1] = dir1[2]*dir2[0] - dir1[0]*dir2[2];
	cross[2] = dir1[0]*dir2[1] - dir1[1]*dir2[0];

	/*
	std::cout << res.firstHorizontal->code() << ":   H1: " << dir1[0] << ", " << dir1[1] << ", " << dir1[2] << std::endl;
	std::cout << res.secondHorizontal->code() << ":   H2: " << dir2[0] << ", " << dir2[1] << ", " << dir2[2] << std::endl;
	std::cout << "    Z: " << dirz[0] << ", " << dirz[1] << ", " << dirz[2] << std::endl;
	std::cout << "syn Z: " << cross[0] << ", " << cross[1] << ", " << cross[2] << std::endl;
	*/

	if ( cross[0]*dirz[0] + cross[1]*dirz[1] + cross[2]*dirz[2] > 0 )
		std::swap(res.comps[ThreeComponents::FirstHorizontal], res.comps[ThreeComponents::SecondHorizontal]);
}


Object *copy(Object* obj) {
	CloneVisitor cv;
	obj->accept(&cv);
	return cv.result();
}


///////////////////////////////////////////////////////////////////////////////
// DataModel Diff / Merge
///////////////////////////////////////////////////////////////////////////////

/**
 * Compares the value of a non array property.
 * @param prop property to compare
 * @param o1 first object to compare
 * @param o2 second object to compare
 * @return true if both values are equal or empty, else false
 * @throw TypeException if the property to compare is of class or array type
 */
bool compareNonArrayProperty(const Core::MetaProperty* prop, Object* o1, Object* o2) {
	if ( prop->isArray() )
		throw Core::TypeException("Expected non array property");

	// Property values may be empty and must be casted to its native
	// type since MetaValue does not implement the comparison operator
	bool isSet_o1 = true;
	bool isSet_o2 = true;
	Core::MetaValue v_o1;
	Core::MetaValue v_o2;
	try { v_o1 = prop->read(o1); } catch ( ... ) { isSet_o1 = false; }
	try { v_o2 = prop->read(o2); } catch ( ... ) { isSet_o2 = false; }
	
	if ( !isSet_o1 && !isSet_o2 ) return true;
	if ( v_o1.empty() && v_o2.empty() ) return true;
	if ( !isSet_o1 || v_o1.empty() ) return false;
	if ( v_o1.type() != v_o2.type() ) return false;

	if ( prop->isEnum() || prop->type() == "int")
		return boost::any_cast<int>(v_o1) == boost::any_cast<int>(v_o2);
	if ( prop->type() == "float" )
		return boost::any_cast<double>(v_o1) == boost::any_cast<double>(v_o2);
	if ( prop->type() == "string" )
		return boost::any_cast<std::string>(v_o1) == boost::any_cast<std::string>(v_o2);
	if ( prop->type() == "datetime" )
		return boost::any_cast<Core::Time>(v_o1) == boost::any_cast<Core::Time>(v_o2);
	if ( prop->type() == "boolean" )
		return boost::any_cast<bool>(v_o1) == boost::any_cast<bool>(v_o2);

	Core::BaseObject* bo1 = boost::any_cast<Core::BaseObject*>(v_o1);
	Core::BaseObject* bo2 = boost::any_cast<Core::BaseObject*>(v_o2);
	if ( prop->type() == "ComplexArray") {
		ComplexArray* ca1 = ComplexArray::Cast(bo1);
		ComplexArray* ca2 = ComplexArray::Cast(bo2);
		return ca1->content() == ca2->content();
	}
	if ( prop->type() == "Blob") {
		Blob* ca1 = Blob::Cast(bo1);
		Blob* ca2 = Blob::Cast(bo2);
		return ca1->content() == ca2->content();
	}
	
	throw Core::TypeException("Unexpected type: " + prop->type());
}

/**
 * Compares the className and all index properties of two objects.
 * @param o1 first object to compare
 * @param o2 second object to compare
 * @return true if the className and all index properties of both objects
 * are equal, else false
 */
bool equalsIndex(Object* o1, Object* o2) {
	// compare className
	if ( o1->className() != o2->className() )
		return false;
	
	// iterate over all properties
	for ( size_t i = 0; i < o1->meta()->propertyCount(); ++i ) {
		const Core::MetaProperty* metaProp = o1->meta()->property(i);
		
		// we do only compare index properties
		if ( ! metaProp->isIndex() )
			continue;
		
		if ( metaProp->isClass() )
			throw Core::TypeException(
				"Violation of contract: property " +
				metaProp->name() +
				" is of class type and marked as index");
			
		if ( ! compareNonArrayProperty(metaProp, o1, o2) )
			return false;
	}
	return true;
}

/**
 * Scans a object tree for a particular node. Objects are compared on base of
 * their indexes, @see equalsIndex
 * @param tree object tree to scan
 * @param node item to search for
 * @return pointer to the item within the object tree or NULL if the node was
 * not found
 */
Object* find(Object* tree, Object* node) {
	if ( equalsIndex(tree, node) ) return tree;
	
	Object* result = NULL;

	// recursive scan of all class properties
	for ( size_t i = 0; i < tree->meta()->propertyCount(); ++i ) {
		const Core::MetaProperty* prop = tree->meta()->property(i);
		
		if ( ! prop->isClass() )
			continue;

		if ( prop->isArray() ) {
			for ( size_t a = 0; a < prop->arrayElementCount(tree); ++a ) {
				Core::BaseObject* bo = prop->arrayObject(tree, a);
				if ( ! bo->typeInfo().isTypeOf(Object::TypeInfo()) )
					break;
				result = find(static_cast<Object*>(bo), node);
				if ( result != NULL ) return result;
			}
		}
		else {
			Core::MetaValue value = prop->read(tree);
			if ( value.empty() ) continue;
			Object* child = boost::any_cast<Object*>(value);
			result = find(child, node);
			if ( result != NULL ) return result;
		}
	}	
	return NULL;
}

/**
 * Returns the public id, if any, of an object. The object is casted to
 * PublicObject and a copy of the publicID is returned.
 * @param o the object to retrieve it's publicID from
 * @return PublicObject.publicID or "" if the Cast to PublicObject failed
 */
const std::string getPublicID(Object* o) {
	PublicObject* po = PublicObject::Cast(o);
	return po ? po->publicID() : "";
}

/**
 * Recursively compares two objects and collects all differences.
 * @param o1 first object to compare
 * @param o2 second object to compare
 * @param o1ParentID parent ID of the first object
 * @param diffList list to collect differences in
 * @throw TypeException if any type restriction is violated
 */
void diffRecursive(Object* o1, Object* o2, const std::string& o1ParentID,
                   std::vector<NotifierPtr>& diffList) {
	// Both objects empty, nothing to compare here
	if ( !o1 && !o2 ) return;
	
	// No element on the left -> ADD
	if ( !o1 ) {
		AppendNotifier<std::vector<NotifierPtr> >(
			diffList, OP_ADD, o2, o1ParentID);
		return;
	}

	// No element on the right -> REMOVE
	if ( !o2 ) {
		AppendNotifier<std::vector<NotifierPtr> >(
			diffList, OP_REMOVE, o1, o1ParentID);
		return;
	}
	
	std::string id_o1 = getPublicID(o1);
	NotifierPtr n;
	// Iterate over all properties
	for ( size_t i = 0; i < o1->meta()->propertyCount(); ++i ) {
		const Core::MetaProperty* prop = o1->meta()->property(i);
		
		// Non array property: Compare simple value(s)
		if ( !prop->isArray() ) {
			if ( !compareNonArrayProperty(prop, o1, o2) ) {
				n = new Notifier(o1ParentID, OP_UPDATE, o2);
				diffList.push_back(n);
			}
			continue;
		}
		
		// Class and array property:
		// The order of elements of a class array is arbitrary, hence
		// each element of one array must be searched among all elements
		// of the other array. A temporary vector with elements of o2 is
		// used to mark previously found matchings.
		std::vector<Object*> v_o2;
		for ( size_t i_o2 = 0; i_o2 < prop->arrayElementCount(o2); ++i_o2 ) {
			const Core::BaseObject* bo = prop->arrayObject(o2, i_o2);
			v_o2.push_back(Object::Cast(const_cast<Core::BaseObject*>(bo)));
		}
		// For each element of o1 array search counterpart in o2
		for ( size_t i_o1 = 0; i_o1 < prop->arrayElementCount(o1); ++i_o1 ) {
			const Core::BaseObject* bo = prop->arrayObject(o1, i_o1);
			Object* o1Child = Object::Cast(const_cast<Core::BaseObject*>(bo));
			std::vector<Object*>::iterator it_o2 = v_o2.begin();
			bool found = false;
			for ( ; it_o2 != v_o2.end(); ++it_o2) {
				if ( equalsIndex(o1Child, *it_o2) ) {
					found = true;
					break;
				}
			}
			if ( found ) {
				diffRecursive(o1Child, *it_o2, id_o1, diffList);
				v_o2.erase(it_o2);
			}
			else {
				AppendNotifier<std::vector<NotifierPtr> >(
					diffList, OP_REMOVE, o1Child, id_o1);
			}
		}

		// Add all elements of o2 array which have no counterpart in o1
		std::vector<Object*>::iterator it_o2 = v_o2.begin();
		for ( ; it_o2 != v_o2.end(); ++it_o2 ) {
			AppendNotifier<std::vector<NotifierPtr> >(diffList, OP_ADD, *it_o2);
		}
		v_o2.clear();
	}
	
}


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
bool diff(Object* o1, Object* o2, std::vector<NotifierPtr>& diffList) {
	if ( !o1 || !o2 )
		return false;

	// Find a common node
	Object* fO1 = find(o1, o2);
	Object* fO2 = !fO1 ? find(o2, o1) : NULL; 
	
	// No common node, bye
	if ( !fO1 && !fO2 ) return false;
	
	o1 = fO1 ? fO1 : o1;
	o2 = fO2 ? fO2 : o2;
	std::string parentID = o1->parent() ? getPublicID(o1->parent()) : "";
	
	// Recursively diff both objects
	diffRecursive(o1, o2, parentID, diffList);
	return true;
}

/**
 * Visitor which collects publicIDs.
 */
class SC_CORE_DATAMODEL_API PublicIDCollector : protected Visitor {
	public:
		void collect(Object *o, std::vector<std::string> *ids) {
			if ( o == NULL || ids == NULL ) return;
			_publicIDs = ids;
			o->accept(this);
			_publicIDs = NULL;
		}

	private:
		bool visit(PublicObject* po) {
			_publicIDs->push_back(po->publicID());
			return true;
		}

		void visit(Object* o) {}

	private:
		std::vector<std::string> *_publicIDs;
};

/**
 * Visitor which validates/repairs all references
 * (MetaProperty::isReference() == true) to publicIDs.
 */
class SC_CORE_DATAMODEL_API ReferenceValidator : public Visitor {

	public:
		/**
		 * Validates all references.
		 * @param o Object to traverse
		 * @param idVector Vector of publicIDs to compare the references
		 *        with
		 * @return true if for each reference a matching entry in the
		 *         idVector was found, else false
		 */
		bool validate(Object *o, const std::vector<std::string> *idVector) {
			if ( o == NULL || idVector == NULL ) return false;
			_validates = idVector;
			_mappings = NULL;
			_valid = true;
			o->accept(this);
			return _valid;
		}
		
		/**
		 * Repairs references.
		 * @param o Object to traverse
		 * @param idMap Mapping of deprecated to current publicIDs
		 */
		size_t repair(Object *o, const std::map<std::string, std::string> *idMap) {
			if ( o == NULL || idMap == NULL ) return 0;
			_validates = NULL;
			_mappings = idMap;
			_mapCount = 0;
			o->accept(this);
			return _mapCount;
		}

	private:
		bool visit(PublicObject* po) {
			processReferences(po);
			return true;
		}

		virtual void visit(Object* o) {
			processReferences(o);
		}

		void processReferences(Object* o) {
			// iterate over all properties
			for ( size_t i = 0; i < o->meta()->propertyCount(); ++i ) {
				const Core::MetaProperty* prop = o->meta()->property(i);
				if ( !prop->isReference() ) continue;
				
				if ( prop->type() != "string" && prop->type() != "Blob" ) {
					SEISCOMP_WARNING("Skipping invalid reference type '%s' in property '%s'",
					                 prop->type().c_str(), prop->name().c_str());
					continue;
				}

				Core::MetaValue value;
				try { value = prop->read(o); } catch ( ... ) {
					SEISCOMP_WARNING("Could not read value of property '%s'",
					                 prop->name().c_str());
					continue;
				}

				if ( _validates )
					validateReference(prop, value);
				else
					repairReference(o, prop, value);
			}
		}
		
		void validateReference(const Core::MetaProperty *prop, const Core::MetaValue &value) {
			std::string ref;
			bool found = true;

			if ( prop->type() == "string" ) {
				ref = boost::any_cast<std::string>(value);
				found = find(_validates->begin(), _validates->end(), ref) != _validates->end();
			}
			else {
				// Type == Blob -> iterate over string list
				Core::BaseObject* bo = boost::any_cast<Core::BaseObject*>(value);
				Blob* refList = Blob::Cast(bo);
				std::vector<std::string> v;
				Core::fromString<std::string>(v, refList->content());
				std::vector<std::string>::const_iterator it = v.begin();
				for ( ; it != v.end(); ++it ) {
					ref = *it;
					found = find(_validates->begin(), _validates->end(), ref) != _validates->end();
					if ( !found ) break;
				}
			}
			
			_valid = _valid && found;
			if (! found) {
				SEISCOMP_WARNING("Broken reference in property '%s': %s",
				                 prop->name().c_str(), ref.c_str());
			}
		}

		
		void repairReference(Object *o, const Core::MetaProperty *prop, const Core::MetaValue &value) {
			std::map<std::string, std::string>::const_iterator it_map;
			if ( prop->type() == "string" ) {
				it_map = _mappings->find(boost::any_cast<std::string>(value));
				if ( it_map != _mappings->end() ) {
					prop->writeString(o, it_map->second);
					SEISCOMP_DEBUG("Replaced reference in property '%s': %s --> %s",
					               prop->name().c_str(), it_map->first.c_str(),
					               it_map->second.c_str());
					++_mapCount;
				}
			}
			else {
				// Type == Blob -> iterate over string list and
				// lookup each token
				Core::BaseObject* bo = boost::any_cast<Core::BaseObject*>(value);
				Blob* refList = Blob::Cast(bo);
				bool modified = false;
				std::vector<std::string> v;
				Core::fromString<std::string>(v, refList->content());
				
				for ( size_t i = 0; i < v.size(); ++i ) {
					it_map = _mappings->find(v[i]);
					if ( it_map != _mappings->end() ) {
						v[i] = it_map->second;
						++_mapCount;
						modified = true;
						SEISCOMP_DEBUG("Replaced reference in blob property '%s[%ld]': %s --> %s",
						               prop->name().c_str(), (long unsigned int) i,
						               it_map->first.c_str(), it_map->second.c_str());
					}
				}

				// Replace Blob value if modified
				if ( modified )
					refList->setContent(Core::toString<std::string>(v));
			}
		}

	private:
		const std::vector<std::string> *_validates;
		const std::map<std::string, std::string> *_mappings;
		bool _valid;
		size_t _mapCount;
};

/**
 * Visitor which recursively clones an object.
 */
class DeepCloner : protected Visitor {
	public:
		//! Constructor, the TraversalMode of TM_TOPDOWN is fix
                DeepCloner() : Visitor(TM_TOPDOWN) {};

		~DeepCloner() { reset(); }
		
		ObjectPtr clone(Object *o) {
			o->accept(this);
			ObjectPtr retn = _clone;
			reset();
			return retn;
		}

	private:
		void reset() { _clone = NULL; _parents.clear(); }

		bool visit(PublicObject *po) {
			PublicObjectPtr clone = PublicObject::Cast(po->clone());

			commit(clone.get());
			_parents.push_front(clone);

			return true;
		}

		void finished() {
			_parents.pop_front();
		}

		void visit(Object *o) {
			ObjectPtr clone = o->clone();
			commit(clone.get());
		}

		void commit(Object *o) {
			if ( !_parents.empty() ) {
				bool wasEnabled = PublicObject::IsRegistrationEnabled();
				PublicObject::SetRegistrationEnabled(false);
				o->attachTo(_parents.front().get());
				PublicObject::SetRegistrationEnabled(wasEnabled);
			}

			if ( _clone == NULL ) _clone = o;
		}

	private:
		std::deque<PublicObjectPtr> _parents;
		ObjectPtr _clone;
};

/**
 * Recursively merges object o2 into object o1.
 * @param o1 object to merge o2 into
 * @param o2 object to merge into o1
 * @param idMap map that keeps track of any publicID attribute changes applied
 * during the merge
 * @throw ValueException if one of the object pointer is NULL
 * @throw TypeException if both objects are not of same type
 */
void mergeRecursive(Object* o1, Object* o2, std::map<std::string, std::string> &idMap) {
	if ( !o1 || !o2 )
		throw Core::ValueException("Invalid object pointer (NULL)");
	if ( o1->typeInfo() != o2->typeInfo() )
		throw Core::TypeException("Type mismatch");

	// Compare the publicID of PublicObjects and create a map entry if they
	// are distinguished
	if ( o1->typeInfo().isTypeOf(PublicObject::TypeInfo()) ) {
		const std::string &o1PID = PublicObject::Cast(o1)->publicID();
		const std::string &o2PID = PublicObject::Cast(o2)->publicID();
		if ( o1PID != o2PID ) {
			std::map<std::string, std::string>::iterator it = idMap.find(o1PID);
			if ( it != idMap.end() && it->second != o2PID )
				throw Core::ValueException("can't map publicID '" +
					it->first + "' to '" + o2PID +
					"' because it is already mapped to '" + it->second + "'");
			else
				idMap[o1PID] = o2PID;
		}
		
		// Empty the publicID so that it can be overridden by the
		// following assign operation
		PublicObject::Cast(o1)->setPublicID("");
	}
	
	// Copy all non class properties
	o1->assign(o2);
	
	// Iterate over all properties and merge children
	for ( size_t i = 0; i < o1->meta()->propertyCount(); ++i ) {
		const Core::MetaProperty* prop = o1->meta()->property(i);
		
		// Skip non class array property
		if ( !prop->isArray() || !prop->isClass() ) continue;

		// Class array property:
		// The order of elements of a class array is arbitrary, hence
		// each element of one array must be searched among all elements
		// of the other array. A temporary vector with elements of the
		// o2 array is used to mark previously found matchings.
		std::vector<Object*> v_o2;
		for ( size_t i_o2 = 0; i_o2 < prop->arrayElementCount(o2); ++i_o2 ) {
			const Core::BaseObject* bo = prop->arrayObject(o2, i_o2);
			v_o2.push_back(Object::Cast(const_cast<Core::BaseObject*>(bo)));
		}
		// For each element of the o1 array search counterpart in the o2
		// and and merge it
		for ( size_t i_o1 = 0; i_o1 < prop->arrayElementCount(o1); ++i_o1 ) {
			const Core::BaseObject* bo = prop->arrayObject(o1, i_o1);
			Object* o1Child = Object::Cast(const_cast<Core::BaseObject*>(bo));
			std::vector<Object*>::iterator it_o2 = v_o2.begin();
			for ( ; it_o2 != v_o2.end(); ++it_o2) {
				if ( equalsIndex(o1Child, *it_o2) ) {
					mergeRecursive(o1Child, *it_o2, idMap);
					v_o2.erase(it_o2);
					break;
				}
			}
		}

		// Add all elements of o2 array which have no counterpart in the
		// o1 are added to the o1 array.
		DeepCloner dc;
		std::vector<Object*>::iterator it_o2 = v_o2.begin();
		for ( ; it_o2 != v_o2.end(); ++it_o2 ) {
			prop->arrayAddObject(o1, dc.clone(*it_o2).get());
		}
		v_o2.clear();
	}
}


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
bool merge(Object* tree, Object* node, std::map<std::string, std::string>& idMap) {
	if ( !tree || !node ) {
		SEISCOMP_WARNING("Invalid merge objects (NULL)");
		return false;
	}

	// Find a common node
	tree = find(tree, node);
	if ( !tree ) {
		SEISCOMP_WARNING("Invalid merge objects (o2 not child of o1)");
		return false;
	}

	// Recursively merge both objects
	mergeRecursive(tree, node, idMap);
	return true;
}

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
merge(Object* mergeResult, const std::vector<Object*>& objects) {
	if ( mergeResult == NULL ) return false;

	// Track publicID changes
	std::map<std::string, std::string> idMap;

	// Merge all objects of the vector
	bool retn = true;
	std::vector<Object*>::const_iterator it = objects.begin();
	for ( ; it != objects.end(); ++it ) {
		retn = merge(mergeResult, *it, idMap) && retn;
	}

	// Repair references
	size_t mappingsPerformed = mapReferences(mergeResult, idMap);
	SEISCOMP_DEBUG("%ld mappingsPerformed", (unsigned long) mappingsPerformed);

	return retn;
}

/**
 * Validates the internal publicID references of the specified object. In a
 * first step all publicIDs are collected, then the object is traversed top-down
 * and each reference's value is searched in the publicID set.
 * @param o object to validate
 * @return true if all references point to an existing publicID, else false
 */
bool validateReferences(Object* o) {
	// Collect publicIDs
	std::vector<std::string> publicIDs;
	PublicIDCollector pc;
	pc.collect(o, &publicIDs);

	// Validate
	ReferenceValidator rv;
	return rv.validate(o, &publicIDs);
}

/**
 * Maps publicID references of the specified object. While the object is
 * traversed top-down, a lookup for each reference in the publicID map is
 * performed. If a matching entry is found the reference's value is updated.
 * @param o object which references should be mapped
 * @param map publicIDMap of deprecated to current publicIDs
 * @return number of mappings performed
 */
SC_CORE_DATAMODEL_API size_t
mapReferences(Object* o, const std::map<std::string, std::string> &publicIDMap) {
	ReferenceValidator rv;
	return rv.repair(o, &publicIDMap);
}

} // of ns DataModel
} // of ns Seiscomp
