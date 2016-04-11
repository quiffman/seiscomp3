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


#define SEISCOMP_COMPONENT SCEVENT
#include <seiscomp3/logging/log.h>

#include "eventinfo.h"
#include "util.h"

#include <algorithm>


using namespace std;
using namespace Seiscomp;
using namespace Seiscomp::DataModel;
using namespace Seiscomp::Client;


EventInformation::EventInformation(Cache *c, Config *cfg_)
: cache(c), cfg(cfg_), created(false) {
}


EventInformation::~EventInformation() {
	for ( list<DataModel::JournalEntryPtr>::iterator it = journal.begin();
	      it != journal.end(); ++it ) {
		(*it)->detach();
	}
}


EventInformation::EventInformation(Cache *c, Config *cfg_,
                                   DatabaseQuery *q, const string &eventID)
: cache(c), cfg(cfg_) {
	load(q, eventID);
}


EventInformation::EventInformation(Cache *c, Config *cfg_,
                                   DatabaseQuery *q, EventPtr &event)
: cache(c), cfg(cfg_) {
	load(q, event);
}


void EventInformation::load(DatabaseQuery *q, const string &eventID) {
	EventPtr e = Event::Find(eventID);
	if ( !e ) {
		PublicObjectPtr po = q?q->getObject(Event::TypeInfo(), eventID):NULL;
		e = Event::Cast(po);
	}

	load(q, e);
}


void EventInformation::load(DatabaseQuery *q, EventPtr &e) {
	pickIDs.clear();
	picks.clear();

	event = e;
	if ( !event )
		return;

	if ( q ) {
		DatabaseIterator it = q?q->getEventPicksByWeight(event->publicID(), 0.0):DatabaseIterator();
		for ( ; it.valid(); ++it ) {
			PickPtr p = Pick::Cast(it.get());
			if ( !p ) continue;
			pickIDs.insert(p->publicID());
			if ( cfg->maxMatchingPicksTimeDiff >= 0 )
				insertPick(p.get());
		}
	}

	preferredOrigin = NULL;
	preferredMagnitude = NULL;
	preferredFocalMechanism = NULL;

	if ( !event->preferredOriginID().empty() ) {
		preferredOrigin = Origin::Find(event->preferredOriginID());
		if ( !preferredOrigin ) {
			PublicObjectPtr po = q?q->getObject(Origin::TypeInfo(), event->preferredOriginID()):NULL;
			preferredOrigin = Origin::Cast(po);
		}
	}

	if ( preferredOrigin ) {
		if ( !preferredOrigin->arrivalCount() && q )
			q->loadArrivals(preferredOrigin.get());
	
		if ( !preferredOrigin->magnitudeCount() && q )
			q->loadMagnitudes(preferredOrigin.get());
	}


	if ( !event->preferredMagnitudeID().empty() ) {
		preferredMagnitude = Magnitude::Find(event->preferredMagnitudeID());
		if ( !preferredMagnitude ) {
			PublicObjectPtr po = q?q->getObject(Magnitude::TypeInfo(), event->preferredMagnitudeID()):NULL;
			preferredMagnitude = Magnitude::Cast(po);
		}
	}

	if ( !event->preferredFocalMechanismID().empty() ) {
		preferredFocalMechanism = FocalMechanism::Find(event->preferredFocalMechanismID());
		if ( !preferredFocalMechanism ) {
			PublicObjectPtr po = q?q->getObject(FocalMechanism::TypeInfo(), event->preferredFocalMechanismID()):NULL;
			preferredFocalMechanism = FocalMechanism::Cast(po);
		}
	}

	if ( preferredFocalMechanism ) {
		if ( !preferredFocalMechanism->momentTensorCount() && q )
			q->loadMomentTensors(preferredFocalMechanism.get());
	}

	// Read journal for event
	if ( q ) {
		DatabaseIterator dbit = q->getJournal(event->publicID());
		while ( dbit.get() != NULL ) {
			JournalEntryPtr entry = JournalEntry::Cast(dbit.get());
			if ( entry )
				addJournalEntry(entry.get());
			++dbit;
		}
	}
}


void EventInformation::loadAssocations(DataModel::DatabaseQuery *q) {
	if ( !q || !event ) return;

	q->load(event.get());
	//q->loadOriginReferences(event.get());
	//q->loadFocalMechanismReferences(event.get());
	//q->loadEventDescriptions(event.get());
}


size_t EventInformation::matchingPicks(DataModel::Origin *o) const {
	typedef pair<PickAssociation::const_iterator, PickAssociation::const_iterator> PickRange;
	size_t matches = 0;

	if ( cfg->maxMatchingPicksTimeDiff < 0 ) {
		for ( size_t i = 0; i < o->arrivalCount(); ++i ) {
			if ( !o->arrival(i) ) continue;
			// weight = 0 => raus
			if ( Private::arrivalWeight(o->arrival(i)) == 0 ) continue;
			if ( pickIDs.find(o->arrival(i)->pickID()) != pickIDs.end() )
				++matches;
		}
	}
	else {
		// TODO: Implement time based check
		for ( size_t i = 0; i < o->arrivalCount(); ++i ) {
			if ( !o->arrival(i) ) continue;
			if ( Private::arrivalWeight(o->arrival(i)) == 0 ) continue;
			PickPtr p = cache->get<Pick>(o->arrival(i)->pickID());
			if ( !p ) {
				SEISCOMP_WARNING("could not load origin pick %s",
				                 o->arrival(i)->pickID().c_str());
				continue;
			}

			string id = p->waveformID().networkCode() + "." +
			            p->waveformID().stationCode();

			PickRange range = picks.equal_range(id);
			PickAssociation::const_iterator it;
			int hit = 0, cnt = 0;
			for ( it = range.first; it != range.second; ++it ) {
				Pick *cmp = it->second.get();
				++cnt;
				try {
					double diff = fabs((double)(cmp->time().value() - p->time().value()));
					if ( diff <= cfg->maxMatchingPicksTimeDiff )
						++hit;
				}
				catch ( ... ) {}
			}

			// No picks checked, continue
			if ( !hit ) continue;

			if ( cfg->matchingPicksTimeDiffAND ) {
				// Here AND is implemented. The distance to every single pick must
				// lie within the configured threshold
				if ( hit == cnt ) ++matches;
			}
			else {
				// OR, at least one pick must match
				++matches;
			}
		}
	}

	return matches;
}


bool EventInformation::valid() const {
	return event != NULL
	       && preferredOrigin != NULL
	       && !pickIDs.empty();
}


bool EventInformation::associate(DataModel::Origin *o) {
	if ( !event ) return false;

	event->add(new OriginReference(o->publicID()));
	for ( size_t i = 0; i < o->arrivalCount(); ++i ) {
		if ( !o->arrival(i) ) continue;
		if ( Private::arrivalWeight(o->arrival(i)) == 0 ) continue;
		pickIDs.insert(o->arrival(i)->pickID());
		if ( cfg->maxMatchingPicksTimeDiff >= 0 ) {
			PickPtr p = cache->get<Pick>(o->arrival(i)->pickID());
			if ( p )
				insertPick(p.get());
			else
				SEISCOMP_WARNING("could not load event pick %s",
				                 o->arrival(i)->pickID().c_str());
		}
	}

	return true;
}


bool EventInformation::associate(DataModel::FocalMechanism *fm) {
	if ( !event ) return false;

	event->add(new FocalMechanismReference(fm->publicID()));

	return true;
}


bool EventInformation::addJournalEntry(DataModel::JournalEntry *e) {
	journal.push_back(e);

	// Update constraints
	if ( e->action() == "EvPrefMagType" )
		constraints.preferredMagnitudeType = e->parameters();
	else if ( e->action() == "EvPrefOrgID" ) {
		constraints.preferredOriginID = e->parameters();
		constraints.preferredOriginEvaluationMode = Core::None;
	}
	else if ( e->action() == "EvPrefFocMecID" ) {
		constraints.preferredFocalMechanismID = e->parameters();
		constraints.preferredFocalMechanismEvaluationMode = Core::None;
	}
	else if ( e->action() == "EvPrefOrgEvalMode" ) {
		if ( e->parameters().empty() ) {
			constraints.preferredOriginEvaluationMode = Core::None;
			constraints.preferredOriginID = "";
		}
		else {
			DataModel::EvaluationMode em;
			if ( em.fromString(e->parameters().c_str()) ) {
				constraints.preferredOriginID = "";
				constraints.preferredOriginEvaluationMode = em;
			}
			else
				return false;
		}
	}
	else if ( e->action() == "EvPrefOrgAutomatic" ) {
		constraints.preferredOriginEvaluationMode = Core::None;
		constraints.preferredOriginID = "";
	}

	return true;
}


bool EventInformation::setEventName(DataModel::JournalEntry *e, string &error) {
	if ( !event ) {
		error = ":internal:";
		return false;
	}

	// Check for updating an existing event description
	for ( size_t i = 0; i < event->eventDescriptionCount(); ++i ) {
		if ( event->eventDescription(i)->type() != EARTHQUAKE_NAME ) continue;
		event->eventDescription(i)->setText(e->parameters());
		event->eventDescription(i)->update();
		return true;
	}

	// Create a new one
	EventDescriptionPtr ed = new EventDescription(e->parameters(), EARTHQUAKE_NAME);
	if ( !event->add(ed.get()) ) {
		error = ":add:";
		return false;
	}

	return true;
}


bool EventInformation::setEventOpComment(DataModel::JournalEntry *e, string &error) {
	if ( !event ) {
		error = ":internal:";
		return false;
	}

	// Check for updating an existing event description
	for ( size_t i = 0; i < event->commentCount(); ++i ) {
		if ( event->comment(i)->id() != "Operator" ) continue;
		event->comment(i)->setText(e->parameters());

		try {
			event->comment(i)->creationInfo().setModificationTime(Core::Time::GMT());
		}
		catch ( ... ) {
			CreationInfo ci;
			ci.setAuthor(e->sender());
			ci.setModificationTime(Core::Time::GMT());
			event->comment(i)->setCreationInfo(ci);
		}

		event->comment(i)->update();
		return true;
	}

	// Create a new one
	CommentPtr cmt = new Comment();
	cmt->setId("Operator");
	cmt->setText(e->parameters());
	CreationInfo ci;
	ci.setAuthor(e->sender());
	try { ci.setCreationTime(e->created()); }
	catch ( ... ) { ci.setCreationTime(Core::Time::GMT()); }
	cmt->setCreationInfo(ci);

	if ( !event->add(cmt.get()) ) {
		error = ":add:";
		return false;
	}

	return true;
}


void EventInformation::insertPick(Pick *p) {
	string id = p->waveformID().networkCode() + "." + p->waveformID().stationCode();
	picks.insert(PickAssociation::value_type(id, p));
}
