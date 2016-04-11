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




#include "eventtool.h"
#include "util.h"

#include <seiscomp3/logging/filerotator.h>
#include <seiscomp3/logging/channel.h>

#include <seiscomp3/datamodel/pick.h>
#include <seiscomp3/datamodel/magnitude.h>
#include <seiscomp3/datamodel/origin.h>
#include <seiscomp3/datamodel/focalmechanism.h>
#include <seiscomp3/datamodel/momenttensor.h>
#include <seiscomp3/datamodel/eventdescription.h>
#include <seiscomp3/datamodel/inventory.h>
#include <seiscomp3/datamodel/journalentry.h>
#include <seiscomp3/datamodel/notifier.h>
#include <seiscomp3/datamodel/utils.h>

#include <seiscomp3/core/system.h>
#include <seiscomp3/math/geo.h>

#include <boost/bind.hpp>

using namespace std;
using namespace Seiscomp;
using namespace Seiscomp::Core;
using namespace Seiscomp::Client;
using namespace Seiscomp::DataModel;
using namespace Seiscomp::Private;

#define DELAY_CHECK_INTERVAL 10
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
namespace {

void makeUpper(std::string &src) {
	for ( size_t i = 0; i < src.size(); ++i )
		src[i] = toupper(src[i]);
}

const char *PRIORITY_TOKENS[] = {
	"AGENCY", "AUTHOR", "STATUS", "METHOD",
	"PHASES", "PHASES_AUTOMATIC",
	"RMS", "RMS_AUTOMATIC",
	"TIME", "TIME_AUTOMATIC"
};


// Global region class defining a rectangular region
// by latmin, lonmin, latmax, lonmax.
DEFINE_SMARTPOINTER(GlobalRegion);
struct GlobalRegion : public Client::Config::Region {
	GlobalRegion() {}

	bool init(const Seiscomp::Config &config, const std::string &prefix) {
		vector<double> region;
		try { region = config.getDoubles(prefix + "rect"); }
		catch ( ... ) {
			return false;
		}

		// Parse region
		if ( region.size() != 4 ) {
			SEISCOMP_ERROR("%srect: expected 4 values in region definition, got %d",
			               prefix.c_str(), (int)region.size());
			return false;
		}

		latMin = region[0];
		lonMin = region[1];
		latMax = region[2];
		lonMax = region[3];

		return true;
	}

	bool isInside(double lat, double lon) const {
		double len, dist;

		if ( lat < latMin || lat > latMax ) return false;

		len = lonMax - lonMin;
		if ( len < 0 )
			len += 360.0;

		dist = lon - lonMin;
		if ( dist < 0 )
			dist += 360.0;

		return dist <= len;
	}

	double latMin, lonMin;
	double latMax, lonMax;
};


}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EventTool::EventTool(int argc, char **argv) : Application(argc, argv) {
	_fExpiry = 1.0; // one hour cache initially

	setAutoApplyNotifierEnabled(true);
	setInterpretNotifierEnabled(true);

	setLoadRegionsEnabled(true);

	setPrimaryMessagingGroup("EVENT");

	addMessagingSubscription("LOCATION");
	addMessagingSubscription("MAGNITUDE");
	addMessagingSubscription("FOCMECH");

	_cache.setPopCallback(boost::bind(&EventTool::removedFromCache, this, _1));

	_infoChannel = SEISCOMP_DEF_LOGCHANNEL("processing/info", Logging::LL_INFO);
	_infoOutput = new Logging::FileRotatorOutput(Environment::Instance()->logFile("scevent-processing-info").c_str(),
	                                             60*60*24, 30);
	_infoOutput->subscribe(_infoChannel);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EventTool::~EventTool() {
	delete _infoChannel;
	delete _infoOutput;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventTool::createCommandLineDescription() {
	Application::createCommandLineDescription();

	commandline().addOption("Messaging", "test", "Test mode, no messages are sent");
	commandline().addOption("Database", "db-disable", "Do not use the database at all");
	commandline().addOption("Generic", "expiry,x", "Time span in hours after which objects expire", &_fExpiry, true);
	commandline().addOption("Generic", "origin-id,O", "Origin ID to associate (local only)", &_originID, true);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool EventTool::validateParameters() {
	_testMode = commandline().hasOption("test");

	if ( commandline().hasOption("db-disable") )
		setDatabaseEnabled(false, false);

	if ( _originID != "" )
		setMessagingEnabled(false);

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool EventTool::initConfiguration() {
	if ( !Application::initConfiguration() )
		return false;

	try { _config.minStationMagnitudes = configGetInt("eventAssociation.minimumMagnitudes"); } catch (...) {}
	try { _config.minMatchingPicks = configGetInt("eventAssociation.minimumMatchingArrivals"); } catch (...) {}
	try { _config.maxMatchingPicksTimeDiff = configGetDouble("eventAssociation.maximumMatchingArrivalTimeDiff"); } catch (...) {}
	try { _config.matchingPicksTimeDiffAND = configGetBool("eventAssociation.compareAllArrivalTimes"); } catch (...) {}
	try { _config.minAutomaticArrivals = configGetInt("eventAssociation.minimumDefiningPhases"); } catch (...) {}

	Config::RegionFilter regionFilter;
	GlobalRegionPtr region = new GlobalRegion;
	if ( region->init(configuration(), "eventAssociation.region.") ) {
		SEISCOMP_INFO("Region check activated");
		regionFilter.region = region;
	}

	try { regionFilter.minDepth = configGetDouble("eventAssociation.region.minDepth"); } catch ( ... ) {}
	try { regionFilter.maxDepth = configGetDouble("eventAssociation.region.maxDepth"); } catch ( ... ) {}

	_config.regionFilter.push_back(regionFilter);

	try { _config.eventTimeBefore = TimeSpan(configGetInt("eventAssociation.eventTimeBefore")); } catch (...) {}
	try { _config.eventTimeAfter = TimeSpan(configGetInt("eventAssociation.eventTimeAfter")); } catch (...) {}
	try { _config.maxTimeDiff = TimeSpan(configGetInt("eventAssociation.maximumTimeSpan")); } catch (...) {}
	try { _config.maxDist = configGetDouble("eventAssociation.maximumDistance"); } catch (...) {}

	try { _config.minMwCount = configGetInt("eventAssociation.minMwCount"); } catch (...) {}

	try { _config.mbOverMwCount = configGetInt("eventAssociation.mbOverMwCount"); } catch (...) {}
	try { _config.mbOverMwValue = configGetDouble("eventAssociation.mbOverMwValue"); } catch (...) {}

	try { _config.eventIDPrefix = configGetString("eventIDPrefix"); } catch (...) {}
	try { _config.eventIDPattern = configGetString("eventIDPattern"); } catch (...) {}

	try { _config.enableFallbackPreferredMagnitude = configGetBool("eventAssociation.enableFallbackMagnitude"); } catch (...) {}
	try { _config.magTypes = configGetStrings("eventAssociation.magTypes"); } catch (...) {}
	try { _config.agencies = configGetStrings("eventAssociation.agencies"); } catch (...) {}
	try { _config.authors = configGetStrings("eventAssociation.authors"); } catch (...) {}
	try { _config.methods = configGetStrings("eventAssociation.methods"); } catch (...) {}
	try { _config.priorities = configGetStrings("eventAssociation.priorities"); } catch (...) {}

	for ( Config::StringList::iterator it = _config.priorities.begin();
	      it != _config.priorities.end(); ++it ) {
		bool validToken = false;
		makeUpper(*it);
		for ( unsigned int t = 0; t < sizeof(PRIORITY_TOKENS) / sizeof(char*); ++t ) {
			if ( *it == PRIORITY_TOKENS[t] ) {
				validToken = true;
				break;
			}
		}

		if ( !validToken ) {
			SEISCOMP_ERROR("Unexpected token in eventAssociation.priorities: %s", it->c_str());
			return false;
		}
	}

	try { _config.delayTimeSpan = configGetInt("eventAssociation.delayTimeSpan"); } catch (...) {}
	try { _config.delayFilter.agencyID = configGetString("eventAssociation.delayFilter.agencyID"); } catch (...) {}
	try { _config.delayFilter.author = configGetString("eventAssociation.delayFilter.author"); } catch (...) {}
	try {
		DataModel::EvaluationMode mode;
		string strMode = configGetString("eventAssociation.delayFilter.evaluationMode");
		if ( !mode.fromString(strMode.c_str()) ) {
			SEISCOMP_ERROR("eventAssociation.delayFilter.evaluationMode: invalid mode");
			return false;
		}

		_config.delayFilter.evaluationMode = mode;
	} catch (...) {}

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool EventTool::init() {
	_config.eventTimeBefore = TimeSpan(30*60);
	_config.eventTimeAfter = TimeSpan(30*60);

	_config.eventIDPrefix = "gfz";
	_config.eventIDPattern = "%p%Y%04c";
	_config.minAutomaticArrivals = 10;
	_config.minStationMagnitudes = 4;
	_config.minMatchingPicks = 3;
	_config.maxMatchingPicksTimeDiff = -1;
	_config.maxTimeDiff = Core::TimeSpan(60.);
	_config.maxDist = 5.0;
	_config.minMwCount = 8;

	_config.mbOverMwCount = 30;
	_config.mbOverMwValue = 6.0;

	_config.enableFallbackPreferredMagnitude = false;

	_config.magTypes.push_back("mBc");
	_config.magTypes.push_back("Mw(mB)");
	_config.magTypes.push_back("Mwp");
	_config.magTypes.push_back("ML");
	_config.magTypes.push_back("MLh");
	_config.magTypes.push_back("MLv");
	_config.magTypes.push_back("mb");

	_config.delayTimeSpan = 0;

	if ( !Application::init() ) return false;

	_inputOrigin = addInputObjectLog("origin");
	_inputMagnitude = addInputObjectLog("magnitude");
	_inputFocalMechanism = addInputObjectLog("focmech");
	_inputMomentTensor = addInputObjectLog("mt");
	_inputOriginRef = addInputObjectLog("originref");
	_inputFMRef = addInputObjectLog("focmechref");
	_inputEvent = addInputObjectLog("event");
	_inputJournal = addInputObjectLog("journal");
	_outputEvent = addOutputObjectLog("event", primaryMessagingGroup());
	_outputOriginRef = addOutputObjectLog("originref", primaryMessagingGroup());
	_outputFMRef = addOutputObjectLog("focmechref", primaryMessagingGroup());

	if ( _config.delayTimeSpan > 0 )
		enableTimer(DELAY_CHECK_INTERVAL);

	_cache.setTimeSpan(TimeSpan(_fExpiry*3600.));
	_cache.setDatabaseArchive(query());

	_ep = new EventParameters;
	_journal = new Journaling;

	EventProcessorFactory::ServiceNames *services;
	services = EventProcessorFactory::Services();

	if ( services ) {
		EventProcessorFactory::ServiceNames::iterator it;

		for ( it = services->begin(); it != services->end(); ++it ) {
			EventProcessorPtr proc = EventProcessorFactory::Create(it->c_str());
			if ( proc ) {
				if ( !proc->setup(configuration()) ) {
					SEISCOMP_WARNING("Event processor '%s' failed to initialize: skipping",
					                 it->c_str());
					continue;
				}

				SEISCOMP_INFO("Processor '%s' added", it->c_str());
				_processors[*it] = proc;
			}
		}

		delete services;
	}

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool EventTool::run() {
	if ( !_originID.empty() ) {
		if ( !query() ) {
			std::cerr << "No database connection available" << std::endl;
			return false;
		}

		OriginPtr origin = Origin::Cast(query()->getObject(Origin::TypeInfo(), _originID));
		if ( !origin ) {
			std::cout << "Origin " << _originID << " has not been found, exiting" << std::endl;
			return true;
		}

		query()->loadArrivals(origin.get());

		EventInformationPtr info = associateOrigin(origin.get(), true);
		if ( !info ) {
			std::cout << "Origin " << _originID << " has not been associated to any event (already associated?)" << std::endl;
			return true;
		}

		std::cout << "Origin " << _originID << " has been associated to event " << info->event->publicID() << std::endl;
		return true;
	}

	return Application::run();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventTool::handleMessage(Core::Message *msg) {
	_adds.clear();
	_updates.clear();
	_realUpdates.clear();
	_originBlackList.clear();

	Application::handleMessage(msg);

	SEISCOMP_DEBUG("Work on TODO list");
	for ( TodoList::iterator it = _adds.begin(); it != _adds.end(); ++it ) {
		SEISCOMP_DEBUG("Check ID %s", (*it)->publicID().c_str());
		OriginPtr org = Origin::Cast(it->get());
		if ( org ) {
			if ( _originBlackList.find(org->publicID()) == _originBlackList.end() ) {
				SEISCOMP_DEBUG("* work on new origin %s (%ld, %d/%lu)",
				               org->publicID().c_str(), (long int)org.get(),
				               definingPhaseCount(org.get()), (unsigned long)org->arrivalCount());
				associateOriginCheckDelay(org.get());
			}
			else
				SEISCOMP_DEBUG("* skipped new origin %s: blacklisted", org->publicID().c_str());

			continue;
		}

		FocalMechanismPtr fm = FocalMechanism::Cast(it->get());
		if ( fm ) {
			SEISCOMP_DEBUG("* work on new focalmechanism %s",
			               fm->publicID().c_str());
			associateFocalMechanism(fm.get());

			continue;
		}

		SEISCOMP_DEBUG("* unhandled object of class %s", (*it)->className());
	}

	for ( TodoList::iterator it = _updates.begin(); it != _updates.end(); ++it ) {
		// Has this object already been added in the previous step or delayed?
		bool delayed = false;
		for ( DelayBuffer::reverse_iterator dit = _delayBuffer.rbegin();
		      dit != _delayBuffer.rend(); ++dit ) {
			if ( dit->first == it->get() ) {
				delayed = true;
				break;
			}
		}

		if ( !delayed && (_adds.find(*it) == _adds.end()) ) {
			OriginPtr org = Origin::Cast(it->get());
			if ( org ) {
				if ( _originBlackList.find(org->publicID()) == _originBlackList.end() )
					updatedOrigin(org.get(), Magnitude::Cast(it->triggered.get()),
					              _realUpdates.find(*it) != _realUpdates.end());
				else
					SEISCOMP_DEBUG("* skipped origin %s: blacklisted", org->publicID().c_str());

				continue;
			}

			FocalMechanismPtr fm = FocalMechanism::Cast(it->get());
			if ( fm ) {
				updatedFocalMechanism(fm.get());
				continue;
			}
		}
	}

	NotifierMessagePtr nmsg = Notifier::GetMessage(true);
	if ( nmsg ) {
		SEISCOMP_DEBUG("%d notifier available", (int)nmsg->size());
		if ( !_testMode ) connection()->send(nmsg.get());
	}
	else
		SEISCOMP_DEBUG("No notifier available");
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventTool::handleTimeout() {
	// First pass: decrease delay time and try to associate
	for ( DelayBuffer::iterator it = _delayBuffer.begin();
	      it != _delayBuffer.end(); ) {
		it->second -= DELAY_CHECK_INTERVAL;
		if ( it->second <= 0 ) {
			OriginPtr org = Origin::Cast(it->first);
			if ( org ) {
				SEISCOMP_LOG(_infoChannel, "Processing delayed origin %s",
				             org->publicID().c_str());
				associateOrigin(org.get(), true);
			}

			FocalMechanismPtr fm = FocalMechanism::Cast(it->first);
			if ( fm ) {
				SEISCOMP_LOG(_infoChannel, "Processing delayed focalmechanism %s",
				             fm->publicID().c_str());
				associateFocalMechanism(fm.get());
			}

			it = _delayBuffer.erase(it);
		}
		else
			++it;
	}

	// Second pass: flush pending origins (if possible)
	for ( DelayBuffer::iterator it = _delayBuffer.begin();
	      it != _delayBuffer.end(); ) {
		OriginPtr org = Origin::Cast(it->first);
		EventInformationPtr info;
		if ( org ) {
			SEISCOMP_LOG(_infoChannel, "Processing delayed origin %s",
			             org->publicID().c_str());
			info = associateOrigin(org.get(), false);
		}

		// Has been associated
		if ( info )
			it = _delayBuffer.erase(it);
		else
			++it;
	}

	NotifierMessagePtr nmsg = Notifier::GetMessage(true);
	if ( nmsg ) {
		SEISCOMP_DEBUG("%d notifier available", (int)nmsg->size());
		if ( !_testMode ) connection()->send(nmsg.get());
	}
	else
		SEISCOMP_DEBUG("No notifier available");
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventTool::addObject(const string &parentID, Object* object) {
	OriginPtr org = Origin::Cast(object);
	if ( org ) {
		logObject(_inputOrigin, Core::Time::GMT());
		SEISCOMP_DEBUG("* queued new origin %s (%ld, %d/%lu)",
		              org->publicID().c_str(), (long int)org.get(),
		              definingPhaseCount(org.get()), (unsigned long)org->arrivalCount());
		SEISCOMP_LOG(_infoChannel, "Received new origin %s", org->publicID().c_str());
		_adds.insert(TodoEntry(org));
		return;
	}

	Magnitude *mag = Magnitude::Cast(object);
	if ( mag ) {
		logObject(_inputMagnitude, Core::Time::GMT());
		org = _cache.get<Origin>(parentID);
		if ( org ) {
			if ( org != mag->origin() )
				org->add(mag);

			SEISCOMP_LOG(_infoChannel, "Received new magnitude %s (%s %.2f)",
			             mag->publicID().c_str(), mag->type().c_str(), mag->magnitude().value());

			_updates.insert(TodoEntry(org));
		}
		return;
	}

	FocalMechanismPtr fm = FocalMechanism::Cast(object);
	if ( fm ) {
		logObject(_inputFocalMechanism, Core::Time::GMT());
		SEISCOMP_DEBUG("* queued new focalmechanism %s (%ld)",
		               fm->publicID().c_str(), (long int)fm.get());
		SEISCOMP_LOG(_infoChannel, "Received new focalmechanism %s", fm->publicID().c_str());
		_adds.insert(TodoEntry(fm));
		return;
	}

	MomentTensor *mt = MomentTensor::Cast(object);
	if ( mt ) {
		logObject(_inputMomentTensor, Core::Time::GMT());
		fm = _cache.get<FocalMechanism>(parentID);
		if ( fm ) {
			if ( fm != mt->focalMechanism() )
				fm->add(mt);

			SEISCOMP_LOG(_infoChannel, "Received new momenttensor %s",
			             mt->publicID().c_str());

			// Blacklist the derived originID to prevent event
			// association.
			_originBlackList.insert(mt->derivedOriginID());
		}
		return;
	}


	OriginReference *ref = OriginReference::Cast(object);
	if ( ref && !ref->originID().empty() ) {
		logObject(_inputOriginRef, Core::Time::GMT());
		SEISCOMP_LOG(_infoChannel, "Received new origin reference %s for event %s",
		             ref->originID().c_str(), parentID.c_str());

		EventInformationPtr info = cachedEvent(parentID);
		if ( !info ) {
			info = new EventInformation(&_cache, &_config, query(), parentID);
			if ( !info->event ) {
				SEISCOMP_ERROR("event %s for OriginReference not found", parentID.c_str());
				SEISCOMP_LOG(_infoChannel, " - skipped, event %s not found in database", parentID.c_str());
				return;
			}

			info->loadAssocations(query());
			cacheEvent(info);
		}

		org = _cache.get<Origin>(ref->originID());

		TodoList::iterator it = _adds.find(TodoEntry(org));
		// If this origin has to be associated in this turn
		if ( it != _adds.end() ) {
			// Remove the origin from the association list
			_adds.erase(it);
			// Add it to the origin updates (not triggered by a magnitude change)
			_realUpdates.insert(TodoEntry(org));
			SEISCOMP_DEBUG("* removed new origin %s from queue because of preset association", (*it)->publicID().c_str());
		}

		_updates.insert(TodoEntry(org));

		return;
	}

	FocalMechanismReference *fm_ref = FocalMechanismReference::Cast(object);
	if ( fm_ref && !fm_ref->focalMechanismID().empty() ) {
		logObject(_inputFMRef, Core::Time::GMT());
		SEISCOMP_LOG(_infoChannel, "Received new focalmechanism reference %s for event %s",
		             fm_ref->focalMechanismID().c_str(), parentID.c_str());

		EventInformationPtr info = cachedEvent(parentID);
		if ( !info ) {
			info = new EventInformation(&_cache, &_config, query(), parentID);
			if ( !info->event ) {
				SEISCOMP_ERROR("event %s for ForcalMechanismReference not found", parentID.c_str());
				SEISCOMP_LOG(_infoChannel, " - skipped, event %s not found in database", parentID.c_str());
				return;
			}

			info->loadAssocations(query());
			cacheEvent(info);
		}

		fm = _cache.get<FocalMechanism>(fm_ref->focalMechanismID());

		TodoList::iterator it = _adds.find(TodoEntry(fm));
		// If this origin has to be associated in this turn
		if ( it != _adds.end() ) {
			// Remove the focalmechanism from the association list
			_adds.erase(it);
			// Add it to the focalmechanism updates (not triggered by a magnitude change)
			_realUpdates.insert(TodoEntry(fm));
			SEISCOMP_DEBUG("* removed new focalmechanism %s from queue because of preset association", (*it)->publicID().c_str());
		}

		_updates.insert(TodoEntry(fm));

		return;
	}

	EventPtr evt = Event::Cast(object);
	if ( evt ) {
		logObject(_inputEvent, Core::Time::GMT());
		SEISCOMP_LOG(_infoChannel, "Received new event %s", evt->publicID().c_str());
		EventInformationPtr info = cachedEvent(evt->publicID());
		if ( !info ) {
			info = new EventInformation(&_cache, &_config, query(), evt);
			// Loading the references does not make sense here, because
			// the event has been just added
			cacheEvent(info);
		}
		return;
	}

	JournalEntryPtr journalEntry = JournalEntry::Cast(object);
	if ( journalEntry ) {
		logObject(_inputJournal, Core::Time::GMT());
		SEISCOMP_LOG(_infoChannel, "Received new journal entry for object %s", journalEntry->objectID().c_str());
		if ( handleJournalEntry(journalEntry.get()) )
			return;
	}

	// We are not interested in anything else than the parsed
	// objects
	if ( object->parent() == _ep.get() || object->parent() == _journal.get() )
		object->detach();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventTool::updateObject(const std::string &parentID, Object* object) {
	OriginPtr org = Origin::Cast(object);
	if ( org ) {
		logObject(_inputOrigin, Core::Time::GMT());
		_updates.insert(TodoEntry(org));
		_realUpdates.insert(TodoEntry(org));
		SEISCOMP_DEBUG("* queued updated origin %s (%d/%lu)",
		               org->publicID().c_str(),
		               definingPhaseCount(org.get()), (unsigned long)org->arrivalCount());
		SEISCOMP_LOG(_infoChannel, "Received updated origin %s", org->publicID().c_str());
		return;
	}

	FocalMechanismPtr fm = FocalMechanism::Cast(object);
	if ( fm ) {
		logObject(_inputFocalMechanism, Core::Time::GMT());
		_updates.insert(TodoEntry(fm));
		_realUpdates.insert(TodoEntry(fm));
		SEISCOMP_DEBUG("* queued updated focalmechanism %s",
		               fm->publicID().c_str());
		SEISCOMP_LOG(_infoChannel, "Received updated focalmechanism %s", fm->publicID().c_str());
		return;
	}

	MagnitudePtr mag = Magnitude::Cast(object);
	if ( mag ) {
		logObject(_inputMagnitude, Core::Time::GMT());
		SEISCOMP_LOG(_infoChannel, "Received new magnitude %s (%s %.2f)",
		             mag->publicID().c_str(), mag->type().c_str(), mag->magnitude().value());
		org = _cache.get<Origin>(parentID);
		if ( org )
			_updates.insert(TodoEntry(org, mag));
		return;
	}

	EventPtr evt = Event::Cast(object);
	if ( evt ) {
		logObject(_inputEvent, Core::Time::GMT());
		SEISCOMP_LOG(_infoChannel, "Received updated event %s", evt->publicID().c_str());
		EventInformationPtr info = cachedEvent(evt->publicID());
		if ( !info ) {
			info = new EventInformation(&_cache, &_config, query(), evt);
			info->loadAssocations(query());
			cacheEvent(info);
		}

		// NOTE: What to do with an event update?
		// What we can do is to compare the cached event by the updated
		// one and resent it if other attributes than preferredOriginID
		// and preferredMagnitudeID has changed (e.g. status)
		// That does not avoid race conditions but minimizes them with
		// the current implementation.
		// For now it's getting cached unless it has been done already.
		return;
	}

	if ( object->parent() == _ep.get() || object->parent() == _journal.get() )
		object->detach();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventTool::removeObject(const string&, Object* object) {

}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
namespace {

DataModel::JournalEntryPtr
createEntry(const std::string &id, const std::string &proc,
            const std::string &param) {
	DataModel::JournalEntryPtr e = new DataModel::JournalEntry;
	e->setObjectID(id);
	e->setAction(proc);
	e->setParameters(param);
	e->setCreated(Core::Time::GMT());
	return e;
}

}

bool EventTool::handleJournalEntry(DataModel::JournalEntry *entry) {
	SEISCOMP_INFO("Handling journal entry for event %s: %s(%s)",
	              entry->objectID().c_str(), entry->action().c_str(),
	              entry->parameters().c_str());

	EventInformationPtr info = cachedEvent(entry->objectID());

	if ( !info ) {
		// No chached information yet -> load it and cache it
		info = new EventInformation(&_cache, &_config, query(), entry->objectID());
		if ( !info->event ) {
			SEISCOMP_INFO("event %s for JournalEntry not found", entry->objectID().c_str());
			return false;
		}

		info->loadAssocations(query());
		cacheEvent(info);
	}
	else
		info->addJournalEntry(entry);

	DataModel::JournalEntryPtr response;

	if ( entry->action() == "EvPrefMagType" ) {
		SEISCOMP_DEBUG("...set preferred magnitude type");
		response = createEntry(entry->objectID(), entry->action() + "OK", !info->constraints.preferredMagnitudeType.empty()?info->constraints.preferredMagnitudeType:":automatic:");

		// Choose the new preferred magnitude
		Notifier::Enable();
		choosePreferred(info.get(), info->preferredOrigin.get(), NULL);
		Notifier::Disable();
	}
	else if ( entry->action() == "EvPrefOrgID" ) {
		SEISCOMP_DEBUG("...set preferred origin by ID");

		// Release fixed origin and choose the best one automatically
		if ( info->constraints.preferredOriginID.empty() ) {
			response = createEntry(entry->objectID(), entry->action() + "OK", ":automatic:");

			std::string lastPreferredOriginID = info->event->preferredOriginID();
			for ( size_t i = 0; i < info->event->originReferenceCount(); ++i ) {
				OriginPtr org = _cache.get<Origin>(info->event->originReference(i)->originID());
				if ( !org ) continue;
				if ( !org->magnitudeCount() && query() ) {
					SEISCOMP_DEBUG("... loading magnitudes for origin %s", org->publicID().c_str());
					query()->loadMagnitudes(org.get());
				}
				choosePreferred(info.get(), org.get(), NULL);
			}

			if ( info->event->preferredOriginID() != lastPreferredOriginID ) {
				Notifier::Enable();
				updateEvent(info->event.get());
				Notifier::Disable();
			}
		}
		else {
			if ( info->event->originReference(info->constraints.preferredOriginID) == NULL ) {
				response = createEntry(entry->objectID(), entry->action() + "Failed", ":unreferenced:");
			}
			else {
				OriginPtr org = _cache.get<Origin>(info->constraints.preferredOriginID);
				if ( org ) {
					if ( !org->magnitudeCount() && query() ) {
						SEISCOMP_DEBUG("... loading magnitudes for origin %s", org->publicID().c_str());
						query()->loadMagnitudes(org.get());
					}

					response = createEntry(entry->objectID(), entry->action() + "OK", info->constraints.preferredOriginID);
					Notifier::Enable();
					choosePreferred(info.get(), org.get(), NULL);
					Notifier::Disable();
				}
				else {
					response = createEntry(entry->objectID(), entry->action() + "Failed", ":not available:");
				}
			}
		}
	}
	else if ( entry->action() == "EvPrefOrgEvalMode" ) {
		SEISCOMP_DEBUG("...set preferred origin by evaluation mode");

		DataModel::EvaluationMode em;
		if ( !em.fromString(entry->parameters().c_str()) && !entry->parameters().empty() ) {
			// If a mode is requested but an invalid mode identifier has been submitted
			response = createEntry(entry->objectID(), entry->action() + "Failed", string(":mode '") + entry->parameters() + "' unknown:");
		}
		else {
			// Release fixed origin mode and choose the best one automatically
			if ( !info->constraints.preferredOriginEvaluationMode )
				response = createEntry(entry->objectID(), entry->action() + "OK", ":automatic:");
			else
				response = createEntry(entry->objectID(), entry->action() + "OK", entry->parameters());

			std::string lastPreferredOriginID = info->event->preferredOriginID();
			for ( size_t i = 0; i < info->event->originReferenceCount(); ++i ) {
				OriginPtr org = _cache.get<Origin>(info->event->originReference(i)->originID());
				if ( !org ) continue;
				if ( !org->magnitudeCount() && query() ) {
					SEISCOMP_DEBUG("... loading magnitudes for origin %s", org->publicID().c_str());
					query()->loadMagnitudes(org.get());
				}
				choosePreferred(info.get(), org.get(), NULL);
			}

			if ( info->event->preferredOriginID() != lastPreferredOriginID ) {
				Notifier::Enable();
				updateEvent(info->event.get());
				Notifier::Disable();
			}
		}
	}
	else if ( entry->action() == "EvPrefOrgAutomatic" ) {
		response = createEntry(entry->objectID(), entry->action() + "OK", ":automatic mode:");

		std::string lastPreferredOriginID = info->event->preferredOriginID();
		for ( size_t i = 0; i < info->event->originReferenceCount(); ++i ) {
			OriginPtr org = _cache.get<Origin>(info->event->originReference(i)->originID());
			if ( !org ) continue;
			if ( !org->magnitudeCount() && query() ) {
				SEISCOMP_DEBUG("... loading magnitudes for origin %s", org->publicID().c_str());
				query()->loadMagnitudes(org.get());
			}
			choosePreferred(info.get(), org.get(), NULL);
		}

		if ( info->event->preferredOriginID() != lastPreferredOriginID ) {
			Notifier::Enable();
			updateEvent(info->event.get());
			Notifier::Disable();
		}
	}
	else if ( entry->action() == "EvType" ) {
		SEISCOMP_DEBUG("...set event type");

		EventType et;
		if ( !entry->parameters().empty() && !et.fromString(entry->parameters()) ) {
			response = createEntry(entry->objectID(), entry->action() + "Failed", ":invalid type:");
		}
		else {
			if ( !entry->parameters().empty() ) {
				info->event->setType(et);
				response = createEntry(entry->objectID(), entry->action() + "OK", entry->parameters());
			}
			else {
				info->event->setType(None);
				response = createEntry(entry->objectID(), entry->action() + "OK", ":unset:");
			}
			Notifier::Enable();
			updateEvent(info->event.get());
			Notifier::Disable();
		}
	}
	else if ( entry->action() == "EvName" ) {
		SEISCOMP_DEBUG("...set event name");

		string error;
		Notifier::Enable();
		if ( info->setEventName(entry, error) )
			response = createEntry(entry->objectID(), entry->action() + "OK", entry->parameters());
		else
			response = createEntry(entry->objectID(), entry->action() + "Failed", error);
		Notifier::Disable();
	}
	else if ( entry->action() == "EvOpComment" ) {
		SEISCOMP_DEBUG("...set event operator's comment");

		string error;
		Notifier::Enable();
		if ( info->setEventOpComment(entry, error) )
			response = createEntry(entry->objectID(), entry->action() + "OK", entry->parameters());
		else
			response = createEntry(entry->objectID(), entry->action() + "Failed", error);
		Notifier::Disable();
	}
	else if ( entry->action() == "EvPrefFocMecID" ) {
		SEISCOMP_DEBUG("...set event preferred focal mechanism");

		if ( entry->parameters().empty()) {
			response = createEntry(entry->objectID(), entry->action() + "Failed", ":empty parameter:");
		}
		else {
			if ( !entry->parameters().empty() ) {
				info->event->setPreferredFocalMechanismID(entry->parameters());
				response = createEntry(entry->objectID(), entry->action() + "OK", entry->parameters());
			}
			else {
				info->event->setPreferredFocalMechanismID("");
				response = createEntry(entry->objectID(), entry->action() + "OK", ":unset:");
			}
			Notifier::Enable();
			updateEvent(info->event.get());
			Notifier::Disable();
		}
	}
	else
		response = createEntry(entry->objectID(), entry->action() + "Failed", ":unknown command:");

	if ( response ) {
		info->addJournalEntry(response.get());
		response->setSender(name() + "@" + Core::getHostname());
		Notifier::Enable();
		Notifier::Create(_journal->publicID(), OP_ADD, response.get());
		Notifier::Disable();
	}

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EventInformationPtr EventTool::associateOriginCheckDelay(DataModel::Origin *origin) {
	if ( _config.delayTimeSpan > 0 ) {
		SEISCOMP_LOG(_infoChannel, "Checking delay filter for origin %s",
		             origin->publicID().c_str());

		EvaluationMode mode = AUTOMATIC;
		try {
			mode = origin->evaluationMode();
		}
		catch ( ValueException& ) {}

		if ( _config.delayFilter.agencyID &&
		     objectAgencyID(origin) != *_config.delayFilter.agencyID ) {
			SEISCOMP_LOG(_infoChannel, " * agency does not match (%s != %s), process immediately",
			             objectAgencyID(origin).c_str(), (*_config.delayFilter.agencyID).c_str());
			return associateOrigin(origin, true);
		}

		if ( _config.delayFilter.author &&
		     objectAuthor(origin) != *_config.delayFilter.author ) {
			SEISCOMP_LOG(_infoChannel, " * author does not match (%s != %s), process immediately",
			             objectAuthor(origin).c_str(), (*_config.delayFilter.author).c_str());
			return associateOrigin(origin, true);
		}

		if ( _config.delayFilter.evaluationMode &&
		     mode != *_config.delayFilter.evaluationMode ) {
			SEISCOMP_LOG(_infoChannel, " * evaluationMode does not match (%s != %s), process immediately",
			             mode.toString(), (*_config.delayFilter.evaluationMode).toString());
			return associateOrigin(origin, true);
		}

		// Filter to delay the origin passes
		SEISCOMP_LOG(_infoChannel, "Origin %s delayed", origin->publicID().c_str());
		_delayBuffer.push_back(DelayedObject(origin, _config.delayTimeSpan));

		return NULL;
	}

	return associateOrigin(origin, true);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EventInformationPtr EventTool::associateOrigin(Seiscomp::DataModel::Origin *origin,
                                               bool allowEventCreation) {
	// Default origin status
	EvaluationMode status = AUTOMATIC;
	try {
		status = origin->evaluationMode();
	}
	catch ( Core::ValueException &e ) {
	}

	// Find a matching (cached) event for this origin
	EventInformationPtr info = findMatchingEvent(origin);
	if ( !info ) {
		Core::Time startTime = origin->time().value() - _config.eventTimeBefore;
		Core::Time endTime = origin->time().value() + _config.eventTimeAfter;
		MatchResult bestResult = Nothing;

		SEISCOMP_DEBUG("... search for origin's %s event in database", origin->publicID().c_str());

		if ( query() ) {
			// Look for events in a certain timewindow around the origintime
			DatabaseIterator it = query()->getEvents(startTime, endTime);

			std::vector<EventPtr> fetchedEvents;
			for ( ; *it; ++it ) {
				EventPtr e = Event::Cast(*it);
				assert(e != NULL);
				// Is this event already cached and associated with an information
				// object?
				if ( isEventCached(e->publicID()) ) continue;

				fetchedEvents.push_back(e);
			}

			for ( size_t i = 0; i < fetchedEvents.size(); ++i ) {
				// Load the eventinformation for this event
				EventInformationPtr tmp = new EventInformation(&_cache, &_config, query(), fetchedEvents[i]);
				if ( tmp->valid() ) {
					MatchResult res = compare(tmp.get(), origin);
					if ( res > bestResult ) {
						bestResult = res;
						info = tmp;
					}
				}
			}
		}

		if ( info ) {
			SEISCOMP_LOG(_infoChannel, "Found matching event %s for origin %s (code: %d)",
			             info->event->publicID().c_str(), origin->publicID().c_str(), bestResult);
			SEISCOMP_DEBUG("... found best matching event %s (code: %d)", info->event->publicID().c_str(), bestResult);
			// Load all references
			info->loadAssocations(query());
			cacheEvent(info);
			if ( info->event->originReference(origin->publicID()) != NULL ) {
				SEISCOMP_DEBUG("... origin already associated to event %s", info->event->publicID().c_str());
				SEISCOMP_LOG(_infoChannel, "Origin %s skipped: already associated to event %s",
				             origin->publicID().c_str(), info->event->publicID().c_str());
				info = NULL;
			}
		}
		// Create a new event
		else {
			if ( !allowEventCreation ) return NULL;

			if ( isAgencyIDBlocked(objectAgencyID(origin)) ) {
				SEISCOMP_LOG(_infoChannel, "Origin %s skipped: agencyID blocked and is not allowed to create a new event",
				             origin->publicID().c_str());
				return NULL;
			}

			if ( status == AUTOMATIC
				&& definingPhaseCount(origin) < (int)_config.minAutomaticArrivals ) {
				SEISCOMP_DEBUG("... rejecting automatic origin %s (phaseCount: %d < %lu)",
				               origin->publicID().c_str(), definingPhaseCount(origin), (unsigned long)_config.minAutomaticArrivals);
				SEISCOMP_LOG(_infoChannel, "Origin %s skipped: phaseCount too low (%d < %lu) to create a new event",
				             origin->publicID().c_str(), definingPhaseCount(origin), (unsigned long)_config.minAutomaticArrivals);
				return NULL;
			}

			if ( !checkRegionFilter(_config.regionFilter, origin) )
				return NULL;

			info = createEvent(origin);
			if ( info ) {
				SEISCOMP_INFO("%s: created", info->event->publicID().c_str());
				SEISCOMP_LOG(_infoChannel, "Origin %s created a new event %s",
				             origin->publicID().c_str(), info->event->publicID().c_str());
			}
		}
	}
	else {
		SEISCOMP_DEBUG("... found cached event information %s for origin", info->event->publicID().c_str());
	}

	if ( info ) {
		// Found an event => so associate the origin
		Notifier::Enable();
		if ( !info->associate(origin) ) {
			SEISCOMP_ERROR("Association of origin %s to event %s failed",
			               origin->publicID().c_str(), info->event->publicID().c_str());
			SEISCOMP_LOG(_infoChannel, "Failed to associate origin %s to event %s",
			             origin->publicID().c_str(), info->event->publicID().c_str());
		}
		else {
			logObject(_outputOriginRef, Time::GMT());
			SEISCOMP_INFO("%s: associated origin %s", info->event->publicID().c_str(),
			              origin->publicID().c_str());
			SEISCOMP_LOG(_infoChannel, "Origin %s associated to event %s",
			             origin->publicID().c_str(), info->event->publicID().c_str());
			choosePreferred(info.get(), origin, NULL, true);
		}

		Notifier::Disable();
	}

	_cache.feed(origin);

	return info;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool EventTool::checkRegionFilter(const Config::RegionFilters &fs, const Origin *origin) {
	Config::RegionFilters::const_iterator it;
	for ( it = fs.begin(); it != fs.end(); ++it ) {
		const Config::RegionFilter &f = *it;
		if ( f.region ) {
			try {
				if ( !f.region->isInside(origin->latitude(),
				                         origin->longitude()) ) {
					SEISCOMP_DEBUG("... rejecting automatic origin %s: region "
					               "criterion not met",
					               origin->publicID().c_str());
					SEISCOMP_LOG(_infoChannel, "Origin %s skipped: region criteria not met",
					             origin->publicID().c_str());
					return false;
				}
			}
			catch ( ValueException &exc ) {
				SEISCOMP_DEBUG("...region check exception: %s", exc.what());
				SEISCOMP_LOG(_infoChannel, "Origin %s skipped: region check exception: %s",
					         origin->publicID().c_str(), exc.what());
				return false;
			}
		}

		if ( f.minDepth ) {
			try {
				if ( origin->depth().value() < *f.minDepth ) {
					SEISCOMP_DEBUG("... rejecting automatic origin %s: min depth "
					               "criterion not met",
					               origin->publicID().c_str());
					SEISCOMP_LOG(_infoChannel, "Origin %s skipped: min depth criteria not met",
					             origin->publicID().c_str());
					return false;
				}
			}
			catch ( ... ) {}
		}

		if ( f.maxDepth ) {
			try {
				if ( origin->depth().value() > *f.maxDepth ) {
					SEISCOMP_DEBUG("... rejecting automatic origin %s: max depth "
					               "criterion not met",
					               origin->publicID().c_str());
					SEISCOMP_LOG(_infoChannel, "Origin %s skipped: max depth criteria not met",
					             origin->publicID().c_str());
					return false;
				}
			}
			catch ( ... ) {}
		}
	}

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventTool::updatedOrigin(DataModel::Origin *org,
                              DataModel::Magnitude *mag, bool realOriginUpdate) {
	// Get the cached origin, not the one sent in this message
	// If there is no cached origin the same instance is passed back
	Origin *origin = Origin::Find(org->publicID());
	if ( origin != org )
		*origin = *org;

	EventInformationPtr info = findAssociatedEvent(origin);
	if ( !info ) {
		EventPtr e;
		if ( query() ) {
			SEISCOMP_DEBUG("... search for origin's %s event in database", origin->publicID().c_str());
			e = Event::Cast(query()->getEvent(origin->publicID()));
		}

		if ( !e ) {
			SEISCOMP_DEBUG("... updated origin %s has not been associated yet, doing this",
			               origin->publicID().c_str());
			associateOrigin(origin, true);
			return;
		}

		info = new EventInformation(&_cache, &_config, query(), e);
		info->loadAssocations(query());
		cacheEvent(info);
	}
	else {
		SEISCOMP_DEBUG("... found cached event information %s for origin", info->event->publicID().c_str());
	}

	if ( !origin->arrivalCount() && query() ) {
		SEISCOMP_DEBUG("... loading arrivals for origin %s", origin->publicID().c_str());
		query()->loadArrivals(origin);
	}

	if ( !origin->magnitudeCount() && query() ) {
		SEISCOMP_DEBUG("... loading magnitudes for origin %s", origin->publicID().c_str());
		query()->loadMagnitudes(origin);
	}

	// Cache this origin
	_cache.feed(origin);

	Notifier::Enable();
	choosePreferred(info.get(), origin, mag, realOriginUpdate);
	Notifier::Disable();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EventInformationPtr EventTool::associateFocalMechanismCheckDelay(DataModel::FocalMechanism *fm) {
	if ( _config.delayTimeSpan > 0 ) {
		EvaluationMode mode = AUTOMATIC;
		try {
			mode = fm->evaluationMode();
		}
		catch ( ValueException& ) {}

		if ( _config.delayFilter.agencyID &&
		     objectAgencyID(fm) != *_config.delayFilter.agencyID )
			return associateFocalMechanism(fm);

		if ( _config.delayFilter.author &&
		     objectAuthor(fm) != *_config.delayFilter.author )
			return associateFocalMechanism(fm);

		if ( _config.delayFilter.evaluationMode &&
		     mode != *_config.delayFilter.evaluationMode )
			return associateFocalMechanism(fm);

		// Filter to delay the origin passes
		SEISCOMP_LOG(_infoChannel, "FocalMechanism %s delayed", fm->publicID().c_str());
		_delayBuffer.push_back(DelayedObject(fm, _config.delayTimeSpan));

		return NULL;
	}

	return associateFocalMechanism(fm);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EventInformationPtr EventTool::associateFocalMechanism(FocalMechanism *fm) {
	EventInformationPtr info;

	// Default origin status
	EvaluationMode status = AUTOMATIC;
	try {
		status = fm->evaluationMode();
	}
	catch ( Core::ValueException &e ) {
	}

	OriginPtr triggeringOrigin = _cache.get<Origin>(fm->triggeringOriginID());
	if ( triggeringOrigin == NULL ) {
		SEISCOMP_DEBUG("... triggering origin %s not found, skipping event association",
		               fm->triggeringOriginID().c_str());
		return info;
	}

	info = findAssociatedEvent(triggeringOrigin.get());
	if ( !info ) {
		EventPtr e;
		if ( query() ) {
			SEISCOMP_DEBUG("... search for triggering origin's %s event in database", triggeringOrigin->publicID().c_str());
			e = Event::Cast(query()->getEvent(triggeringOrigin->publicID()));
		}

		if ( !e ) {
			SEISCOMP_DEBUG("... triggering origin %s has not been associated yet, skipping focalmechanism",
			               triggeringOrigin->publicID().c_str());
			return NULL;
		}

		info = new EventInformation(&_cache, &_config, query(), e);
		info->loadAssocations(query());
		cacheEvent(info);
	}
	else {
		SEISCOMP_DEBUG("... found cached event information %s for focalmechanism", info->event->publicID().c_str());
	}

	if ( !fm->momentTensorCount() && query() ) {
		SEISCOMP_DEBUG("... loading moment tensor for focalmechanism %s", fm->publicID().c_str());
		query()->loadMomentTensors(fm);
	}

	// Cache this origin
	_cache.feed(fm);

	Notifier::Enable();
	if ( !info->associate(fm) ) {
		SEISCOMP_ERROR("Association of focalmechanism %s to event %s failed",
		               fm->publicID().c_str(), info->event->publicID().c_str());
		SEISCOMP_LOG(_infoChannel, "Failed to associate focalmechanism %s to event %s",
		             fm->publicID().c_str(), info->event->publicID().c_str());
	}
	else {
		logObject(_outputFMRef, Time::GMT());
		SEISCOMP_INFO("%s: associated focalmechanism %s", info->event->publicID().c_str(),
		              fm->publicID().c_str());
		SEISCOMP_LOG(_infoChannel, "FocalMechanism %s associated to event %s",
		             fm->publicID().c_str(), info->event->publicID().c_str());
		choosePreferred(info.get(), fm);
	}
	Notifier::Disable();

	return info;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventTool::updatedFocalMechanism(FocalMechanism *focalMechanism) {
	// Get the cached origin, not the one sent in this message
	// If there is no cached origin the same instance is passed back
	FocalMechanism *fm = FocalMechanism::Find(focalMechanism->publicID());
	if ( focalMechanism != fm )
		*fm = *focalMechanism;

	EventInformationPtr info = findAssociatedEvent(fm);
	if ( !info ) {
		EventPtr e;
		if ( query() ) {
			SEISCOMP_DEBUG("... search for focal mechanisms's %s event in database", fm->publicID().c_str());
			e = Event::Cast(query()->getEventForFocalMechanism(fm->publicID()));
		}

		if ( !e ) {
			SEISCOMP_DEBUG("... updated focal mechanism %s has not been associated yet, doing this",
			               fm->publicID().c_str());
			associateFocalMechanism(fm);
			return;
		}

		info = new EventInformation(&_cache, &_config, query(), e);
		info->loadAssocations(query());
		cacheEvent(info);
	}
	else {
		SEISCOMP_DEBUG("... found cached event information %s for focalmechanism", info->event->publicID().c_str());
	}

	if ( !fm->momentTensorCount() && query() ) {
		SEISCOMP_DEBUG("... loading moment tensor for focalmechanism %s", fm->publicID().c_str());
		query()->loadMomentTensors(fm);
	}

	// Cache this origin
	_cache.feed(fm);

	Notifier::Enable();
	choosePreferred(info.get(), fm);
	Notifier::Disable();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EventTool::MatchResult EventTool::compare(EventInformation *info,
                                          Seiscomp::DataModel::Origin *origin) {
	size_t matchingPicks = info->matchingPicks(origin);

	MatchResult result = Nothing;

	if ( matchingPicks >= _config.minMatchingPicks )
		result = Picks;

	if ( _config.maxMatchingPicksTimeDiff >= 0 )
		SEISCOMP_DEBUG("... compare pick times with threshold %.2fs",
		               _config.maxMatchingPicksTimeDiff);
	SEISCOMP_DEBUG("... matching picks of %s and %s = %lu/%lu, need at least %lu",
	               origin->publicID().c_str(), info->event->publicID().c_str(),
	               (unsigned long)matchingPicks, (unsigned long)origin->arrivalCount(),
	               (unsigned long)_config.minMatchingPicks);

	if ( !info->preferredOrigin )
		return Nothing;

	double dist, azi1, azi2;

	Math::Geo::delazi(origin->latitude().value(), origin->longitude().value(),
	                  info->preferredOrigin->latitude().value(),
	                  info->preferredOrigin->longitude().value(),
	                  &dist, &azi1, &azi2);

	// Dist out of range
	if ( dist <= _config.maxDist ) {
		TimeSpan diffTime = info->preferredOrigin->time().value() - origin->time().value();

		if ( diffTime.abs() <= _config.maxTimeDiff ) {

			if ( result == Picks )
				result = PicksAndLocation;
			else
				result = Location;

		}
	}

	switch ( result ) {
		case Nothing:
			SEISCOMP_DEBUG("... no match for %s and %s",
			               origin->publicID().c_str(), info->event->publicID().c_str());
			break;
		case Location:
			SEISCOMP_DEBUG("... time/location matches for %s and %s",
			               origin->publicID().c_str(), info->event->publicID().c_str());
			break;
		case Picks:
			SEISCOMP_DEBUG("... matching picks for %s and %s",
			               origin->publicID().c_str(), info->event->publicID().c_str());
			break;
		case PicksAndLocation:
			SEISCOMP_DEBUG("... matching picks and time/location for %s and %s",
			               origin->publicID().c_str(), info->event->publicID().c_str());
			break;
	}

	return result;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EventInformationPtr EventTool::createEvent(Origin *origin) {
	string eventID = allocateEventID(query(), origin, _config.eventIDPrefix,
	                                 _config.eventIDPattern);

	if ( eventID.empty() ) {
		SEISCOMP_ERROR("Unable to allocate a new eventID, skipping origin %s", origin->publicID().c_str());
		SEISCOMP_LOG(_infoChannel, "Event created failed: unable to allocate a new EventID");
		return NULL;
	}
	else {
		Time now = Time::GMT();
		logObject(_outputEvent, now);

		Notifier::Enable();

		EventInformationPtr info = new EventInformation(&_cache, &_config);
		info->event = new Event(eventID);

		CreationInfo ci;
		ci.setAgencyID(agencyID());
		ci.setAuthor(author());
		ci.setCreationTime(now);

		info->event->setCreationInfo(ci);
		info->created = true;

		cacheEvent(info);

		Notifier::Disable();

		return info;
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EventInformationPtr EventTool::findMatchingEvent(Origin *origin) {
	MatchResult bestResult = Nothing;
	EventInformationPtr bestInfo = NULL;

	for ( EventMap::iterator it = _events.begin();
	      it != _events.end(); ++it ) {
		MatchResult res = compare(it->second.get(), origin);
		if ( res > bestResult ) {
			bestResult = res;
			bestInfo = it->second;
		}
	}

	if ( bestInfo )
		SEISCOMP_DEBUG("... found best matching cached event %s (code: %d)", bestInfo->event->publicID().c_str(), bestResult);

	return bestInfo;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EventInformationPtr EventTool::findAssociatedEvent(DataModel::Origin *origin) {
	for ( EventMap::iterator it = _events.begin();
	      it != _events.end(); ++it ) {
		if ( it->second->event->originReference(origin->publicID()) != NULL ) {
			SEISCOMP_DEBUG("... feeding cache with event %s",
			               it->second->event->publicID().c_str());
			_cache.feed(it->second->event.get());
			return it->second;
		}
	}

	return NULL;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EventInformationPtr EventTool::findAssociatedEvent(DataModel::FocalMechanism *fm) {
	for ( EventMap::iterator it = _events.begin();
	      it != _events.end(); ++it ) {
		if ( it->second->event->focalMechanismReference(fm->publicID()) != NULL ) {
			SEISCOMP_DEBUG("... feeding cache with event %s",
			               it->second->event->publicID().c_str());
			_cache.feed(it->second->event.get());
			return it->second;
		}
	}

	return NULL;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Magnitude *EventTool::preferredMagnitude(Origin *origin) {
	int goodCount = 0;
	int goodPriority = 0;
	int fallbackCount = 0;
	int fallbackPriority = 0;
	Magnitude *goodMag = NULL;
	Magnitude *fallbackMag = NULL;

	int mbcount = 0;
	double mbval = 0.0;
	for ( size_t i = 0; i < origin->magnitudeCount(); ++i ) {
		Magnitude *mag = origin->magnitude(i);
		if ( isAgencyIDBlocked(objectAgencyID(mag)) ) continue;
		if ( mag->type() == "mb" ) {
			if ( mag->magnitude().value() > mbval ) {
				mbval = mag->magnitude().value();
				mbcount = stationCount(mag);
			}
		}
	}

	for ( size_t i = 0; i < origin->magnitudeCount(); ++i ) {
		try {
			Magnitude *mag = origin->magnitude(i);
			if ( isAgencyIDBlocked(objectAgencyID(mag)) ) continue;

			int priority = goodness(mag, mbcount, mbval, _config);
			if ( priority <= 0 )
				continue;

			if ( isMw(mag) ) {
				if ( (stationCount(mag) > goodCount)
				  || ((stationCount(mag) == goodCount) && (priority > goodPriority)) ) {
					goodPriority = priority;
					goodCount = stationCount(mag);
					goodMag = mag;
				}
			}
			else if ( (stationCount(mag) > fallbackCount)
			       || ((stationCount(mag) == fallbackCount) && (priority > fallbackPriority)) ) {
				fallbackPriority = priority;
				fallbackCount = stationCount(mag);
				fallbackMag = mag;
			}
		}
		catch ( ValueException& ) {
			continue;
		}
	}

	if ( goodMag )
		return goodMag;

	if ( !fallbackMag && _config.enableFallbackPreferredMagnitude ) {
		// Find the network magnitude with the most station magnitudes according the
		// magnitude priority and set this magnitude preferred
		fallbackCount = 0;
		fallbackPriority = 0;

		for ( size_t i = 0; i < origin->magnitudeCount(); ++i ) {
			Magnitude *mag = origin->magnitude(i);
			if ( isAgencyIDBlocked(objectAgencyID(mag)) ) continue;

			int prio = magnitudePriority(mag->type(), _config);
			if ( (stationCount(mag) > fallbackCount)
			  || (stationCount(mag) == fallbackCount && prio > fallbackPriority ) ) {
				fallbackPriority = prio;
				fallbackCount = stationCount(mag);
				fallbackMag = mag;
			}
		}
	}

	return fallbackMag;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventTool::cacheEvent(EventInformationPtr info) {
	SEISCOMP_DEBUG("... caching event %s",
	               info->event->publicID().c_str());

	// Cache the complete event information
	_events[info->event->publicID()] = info;
	// Add the event to the EventParameters
	_ep->add(info->event.get());
	// Feed event into cache
	_cache.feed(info->event.get());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool EventTool::isEventCached(const string &eventID) const {
	return _events.find(eventID) != _events.end();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EventInformationPtr EventTool::cachedEvent(const std::string &eventID) const {
	EventMap::const_iterator it = _events.find(eventID);
	if ( it == _events.end() ) return NULL;
	return it->second;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventTool::choosePreferred(EventInformation *info, Origin *origin,
                                DataModel::Magnitude *triggeredMag,
                                bool realOriginUpdate) {
	Magnitude *mag = NULL;

	SEISCOMP_DEBUG("%s: [choose preferred origin/magnitude(%s)]",
	               info->event->publicID().c_str(),
	               origin->publicID().c_str());

	// Is originID blacklisted, e.g. a derived origin of a moment tensor
	if ( _originBlackList.find(origin->publicID()) != _originBlackList.end() ) {
		SEISCOMP_DEBUG("%s: skip setting preferredOriginID, origin %s is blacklisted",
		               info->event->publicID().c_str(), origin->publicID().c_str());
		SEISCOMP_LOG(_infoChannel, "Origin %s cannot be set preferred: blacklisted",
		             origin->publicID().c_str());
		return;
	}

	// Special magnitude type requested?
	if ( !info->constraints.preferredMagnitudeType.empty() ) {
		SEISCOMP_DEBUG("... requested preferred magnitude type is %s",
		               info->constraints.preferredMagnitudeType.c_str());
		for ( size_t i = 0; i < origin->magnitudeCount(); ++i ) {
			Magnitude *nm = origin->magnitude(i);
			if ( nm->type() == info->constraints.preferredMagnitudeType ) {
				SEISCOMP_DEBUG("... found magnitude %s with requested type",
				               nm->publicID().c_str());
				mag = nm;
				break;
			}
		}
	}

	// No magnitude found -> go on using the normal procedure
	if ( mag == NULL ) {
		SEISCOMP_DEBUG("... looking for preferred magnitude in origin %s",
		               origin->publicID().c_str());
		mag = preferredMagnitude(origin);
	}

	bool update = false;

	if ( !info->preferredOrigin ) {
		if ( isAgencyIDAllowed(objectAgencyID(origin)) || info->constraints.fixOrigin(origin) ) {
			info->event->setPreferredOriginID(origin->publicID());

			std::string reg = region(origin);
			EventDescription *ed = eventRegionDescription(info->event.get());
			if ( ed != NULL ) {
				if ( ed->text() != reg ) {
					ed->setText(reg);
					ed->update();
				}
			}
			else {
				EventDescriptionPtr ed = new EventDescription(reg, REGION_NAME);
				info->event->add(ed.get());
			}

			info->preferredOrigin = origin;
			SEISCOMP_INFO("%s: set first preferredOriginID to %s",
			              info->event->publicID().c_str(), origin->publicID().c_str());
			SEISCOMP_LOG(_infoChannel, "Origin %s has been set preferred in event %s",
			             origin->publicID().c_str(), info->event->publicID().c_str());
			update = true;
		}
		else {
			if ( !isAgencyIDAllowed(objectAgencyID(origin)) ) {
				SEISCOMP_DEBUG("%s: skip setting first preferredOriginID, agencyID '%s' is blocked",
				               info->event->publicID().c_str(), objectAgencyID(origin).c_str());
				SEISCOMP_LOG(_infoChannel, "Origin %s has not been set preferred: agencyID %s is blocked",
				             origin->publicID().c_str(), objectAgencyID(origin).c_str());
			}
			else {
				SEISCOMP_DEBUG("%s: skip setting first preferredOriginID, preferredOriginID is fixed to '%s'",
				               info->event->publicID().c_str(), info->constraints.preferredOriginID.c_str());
				SEISCOMP_LOG(_infoChannel, "Origin %s has not been set preferred: preferredOriginID is fixed to %s",
				             origin->publicID().c_str(), info->constraints.preferredOriginID.c_str());
			}
			return;
		}

		if ( mag ) {
			info->event->setPreferredMagnitudeID(mag->publicID());
			info->preferredMagnitude = mag;
			SEISCOMP_INFO("%s: set first preferredMagnitudeID to %s",
			              info->event->publicID().c_str(), mag->publicID().c_str());
			SEISCOMP_LOG(_infoChannel, "Magnitude %s has been set preferred in event %s",
			             mag->publicID().c_str(), info->event->publicID().c_str());
			update = true;
		}
	}
	else if ( info->preferredOrigin->publicID() != origin->publicID() ) {
		SEISCOMP_DEBUG("... checking whether origin %s can become preferred",
		               origin->publicID().c_str());

		update = true;

		// Fixed origin => check if the passed origin is the fixed one
		if ( info->constraints.fixedOrigin() ) {
			if ( !info->constraints.fixOrigin(origin) ) {
				SEISCOMP_DEBUG("... skipping potential preferred origin, origin '%s' is fixed",
				               info->constraints.preferredOriginID.c_str());
				SEISCOMP_LOG(_infoChannel, "Origin %s has not been set preferred in event %s: origin %s is fixed",
				             origin->publicID().c_str(), info->event->publicID().c_str(),
				             info->constraints.preferredOriginID.c_str());
				return;
			}
		}
		// No fixed origin => select it using the automatic rules
		else {

			if ( isAgencyIDBlocked(objectAgencyID(origin)) ) {
				SEISCOMP_DEBUG("... skipping potential preferred origin, agencyID '%s' is blocked",
				               objectAgencyID(origin).c_str());
				SEISCOMP_LOG(_infoChannel, "Origin %s has not been set preferred in event %s: agencyID %s is blocked",
				             origin->publicID().c_str(), info->event->publicID().c_str(),
				             objectAgencyID(origin).c_str());
				return;
			}

			if ( !_config.priorities.empty() ) {
				bool allowBadMagnitude = false;

				// Run through the priority list and check the values
				for ( Config::StringList::iterator it = _config.priorities.begin();
				      it != _config.priorities.end(); ++it ) {
					if ( *it == "AGENCY" ) {
						int originAgencyPriority = agencyPriority(objectAgencyID(origin), _config);
						int preferredOriginAgencyPriority = agencyPriority(objectAgencyID(info->preferredOrigin.get()), _config);

						if ( originAgencyPriority < preferredOriginAgencyPriority ) {
							SEISCOMP_DEBUG("... skipping potential preferred origin, priority of agencyID '%s' is too low",
							               objectAgencyID(origin).c_str());
							SEISCOMP_LOG(_infoChannel, "Origin %s has not been set preferred in event %s: priority of agencyID %s is too low",
							             origin->publicID().c_str(), info->event->publicID().c_str(),
							             objectAgencyID(origin).c_str());
							return;
						}
						// Found origin with higher agency priority
						else if ( originAgencyPriority > preferredOriginAgencyPriority ) {
							SEISCOMP_DEBUG("... agencyID '%s' overrides current agencyID '%s'",
							               objectAgencyID(origin).c_str(), objectAgencyID(info->preferredOrigin.get()).c_str());
							SEISCOMP_LOG(_infoChannel, "Origin %s: agencyID '%s' overrides agencyID '%s'",
							             origin->publicID().c_str(), objectAgencyID(origin).c_str(), objectAgencyID(info->preferredOrigin.get()).c_str());

							allowBadMagnitude = true;
							break;
						}
					}
					else if ( *it == "AUTHOR" ) {
						int originAuthorPriority = authorPriority(objectAuthor(origin), _config);
						int preferredOriginAuthorPriority = authorPriority(objectAuthor(info->preferredOrigin.get()), _config);

						if ( originAuthorPriority < preferredOriginAuthorPriority ) {
							SEISCOMP_DEBUG("... skipping potential preferred origin, priority of author '%s' is too low",
							               objectAuthor(origin).c_str());
							SEISCOMP_LOG(_infoChannel, "Origin %s has not been set preferred in event %s: priority of author %s is too low",
							             origin->publicID().c_str(), info->event->publicID().c_str(),
							             objectAuthor(origin).c_str());
							return;
						}
						// Found origin with higher author priority
						else if ( originAuthorPriority > preferredOriginAuthorPriority ) {
							SEISCOMP_DEBUG("... author '%s' overrides current author '%s'",
							               objectAuthor(origin).c_str(), objectAuthor(info->preferredOrigin.get()).c_str());
							SEISCOMP_LOG(_infoChannel, "Origin %s: author '%s' overrides author '%s'",
							             origin->publicID().c_str(), objectAuthor(origin).c_str(), objectAuthor(info->preferredOrigin.get()).c_str());
							break;
						}
					}
					else if ( *it == "STATUS" ) {
						int originPriority = priority(origin);
						int preferredOriginPriority = priority(info->preferredOrigin.get());

						if ( info->constraints.fixOriginMode(info->preferredOrigin.get()) ) {
							SEISCOMP_LOG(_infoChannel, "Origin %s: has priority %d vs %d",
							             origin->publicID().c_str(), originPriority, preferredOriginPriority);

							// Set back the evalmode to automatic if a higher priority
							// origin has been send (but not triggered by a magnitude change only)
							if ( realOriginUpdate && (originPriority > preferredOriginPriority) ) {
								SEISCOMP_LOG(_infoChannel, "Origin %s has higher priority: releasing EvPrefOrgEvalMode",
								             origin->publicID().c_str());
								JournalEntryPtr entry = new JournalEntry;
								entry->setObjectID(info->event->publicID());
								entry->setAction("EvPrefOrgEvalMode");
								entry->setParameters("");
								entry->setSender(name() + "@" + Core::getHostname());
								entry->setCreated(Core::Time::GMT());
								Notifier::Create(_journal->publicID(), OP_ADD, entry.get());
								info->addJournalEntry(entry.get());
							}
							else
								preferredOriginPriority = ORIGIN_PRIORITY_MAX;
						}

						if ( info->constraints.fixOriginMode(origin) )
							originPriority = ORIGIN_PRIORITY_MAX;

						if ( originPriority < preferredOriginPriority ) {
							SEISCOMP_DEBUG("... skipping potential preferred origin (%d < %d)",
							               originPriority, preferredOriginPriority);
							SEISCOMP_LOG(_infoChannel, "Origin %s has not been set preferred in event %s: priority too low (%d < %d)",
							             origin->publicID().c_str(), info->event->publicID().c_str(),
							             originPriority, preferredOriginPriority);
							return;
						}
						// Found origin with higher status priority
						else if ( originPriority > preferredOriginPriority ) {
							SEISCOMP_LOG(_infoChannel, "Origin %s: status priority %d overrides status priority %d",
							             origin->publicID().c_str(), originPriority, preferredOriginPriority);
							break;
						}
					}
					else if ( *it == "METHOD" ) {
						int originMethodPriority = methodPriority(origin->methodID(), _config);
						int preferredOriginMethodPriority = methodPriority(info->preferredOrigin->methodID(), _config);

						if ( originMethodPriority < preferredOriginMethodPriority ) {
							SEISCOMP_DEBUG("... skipping potential preferred origin, priority of method '%s' is too low",
							               origin->methodID().c_str());
							SEISCOMP_LOG(_infoChannel, "Origin %s has not been set preferred in event %s: priority of method %s is too low",
							             origin->publicID().c_str(), info->event->publicID().c_str(),
							             origin->methodID().c_str());
							return;
						}
						// Found origin with higher method priority
						else if ( originMethodPriority > preferredOriginMethodPriority ) {
							SEISCOMP_DEBUG("... methodID '%s' overrides current methodID '%s'",
							               origin->methodID().c_str(), info->preferredOrigin->methodID().c_str());
							SEISCOMP_LOG(_infoChannel, "Origin %s: methodID '%s' overrides methodID '%s'",
							             origin->publicID().c_str(), origin->methodID().c_str(),
							             info->preferredOrigin->methodID().c_str());
							break;
						}
					}
					else if ( *it == "PHASES" ) {
						int originPhaseCount = definingPhaseCount(origin);
						int preferredOriginPhaseCount = definingPhaseCount(info->preferredOrigin.get());
						if ( originPhaseCount < preferredOriginPhaseCount ) {
							SEISCOMP_DEBUG("... skipping potential preferred automatic origin, phaseCount too low");
							SEISCOMP_LOG(_infoChannel, "Origin %s has not been set preferred in event %s: phaseCount too low (%d < %d)",
							             origin->publicID().c_str(), info->event->publicID().c_str(),
							             originPhaseCount, preferredOriginPhaseCount);
							return;
						}
						else if ( originPhaseCount > preferredOriginPhaseCount ) {
							SEISCOMP_LOG(_infoChannel, "Origin %s: phaseCount %d overrides phaseCount %d",
							             origin->publicID().c_str(), originPhaseCount,
							             preferredOriginPhaseCount);
							break;
						}
					}
					else if ( *it == "PHASES_AUTOMATIC" ) {
						EvaluationMode status = AUTOMATIC;
						try {
							status = origin->evaluationMode();
						}
						catch ( ValueException& ) {}

						// Make a noop in case of non automatic origins
						if ( status != AUTOMATIC ) continue;

						int originPhaseCount = definingPhaseCount(origin);
						int preferredOriginPhaseCount = definingPhaseCount(info->preferredOrigin.get());
						if ( originPhaseCount < preferredOriginPhaseCount ) {
							SEISCOMP_DEBUG("... skipping potential preferred automatic origin, phaseCount too low");
							SEISCOMP_LOG(_infoChannel, "Origin %s has not been set preferred in event %s: phaseCount too low (%d < %d)",
							             origin->publicID().c_str(), info->event->publicID().c_str(),
							             originPhaseCount, preferredOriginPhaseCount);
							return;
						}
						else if ( originPhaseCount > preferredOriginPhaseCount ) {
							SEISCOMP_LOG(_infoChannel, "Origin %s: phaseCount (automatic) %d overrides phaseCount %d",
							             origin->publicID().c_str(), originPhaseCount,
							             preferredOriginPhaseCount);
							break;
						}
					}
					else if ( *it == "RMS" ) {
						double originRMS = rms(origin);
						double preferredOriginRMS = rms(info->preferredOrigin.get());

						// Both RMS attributes are set: check priority
						if ( (originRMS >= 0) && (preferredOriginRMS >= 0) && (originRMS > preferredOriginRMS) ) {
							SEISCOMP_DEBUG("... skipping potential preferred automatic origin, RMS too high");
							SEISCOMP_LOG(_infoChannel, "Origin %s has not been set preferred in event %s: RMS too high (%.1f > %.1f",
							             origin->publicID().c_str(), info->event->publicID().c_str(),
							             originRMS, preferredOriginRMS);
							return;
						}
						// Both RMS attribute are set: check priority
						else if ( (originRMS >= 0) && (preferredOriginRMS >= 0) && (originRMS < preferredOriginRMS) ) {
							SEISCOMP_LOG(_infoChannel, "Origin %s: RMS %.1f overrides RMS %.1f",
							             origin->publicID().c_str(), originRMS,
							             preferredOriginRMS);
							break;
						}
						// Incoming RMS is not set but preferred origin has RMS: skip incoming
						else if ( (originRMS < 0) && (preferredOriginRMS >= 0) ) {
							SEISCOMP_LOG(_infoChannel, "Origin %s has not been set preferred in event %s: RMS not set",
							             origin->publicID().c_str(), info->event->publicID().c_str());
							return;
						}
						// Incoming RMS is set but preferred origin has no RMS: prioritize incoming
						else if ( (originRMS >= 0) && (preferredOriginRMS < 0) ) {
							SEISCOMP_LOG(_infoChannel, "Origin %s: RMS %.1f overrides current unset preferred RMS",
							             origin->publicID().c_str(), originRMS);
							break;
						}
					}
					else if ( *it == "RMS_AUTOMATIC" ) {
						EvaluationMode status = AUTOMATIC;
						try {
							status = origin->evaluationMode();
						}
						catch ( ValueException& ) {}

						// Make a noop in case of non automatic origins
						if ( status != AUTOMATIC ) continue;

						double originRMS = rms(origin);
						double preferredOriginRMS = rms(info->preferredOrigin.get());

						// Both RMS attributes are set: check priority
						if ( (originRMS >= 0) && (preferredOriginRMS >= 0) && (originRMS > preferredOriginRMS) ) {
							SEISCOMP_DEBUG("... skipping potential preferred automatic origin, RMS too high");
							SEISCOMP_LOG(_infoChannel, "Origin %s has not been set preferred in event %s: RMS too high (%.1f > %.1f",
							             origin->publicID().c_str(), info->event->publicID().c_str(),
							             originRMS, preferredOriginRMS);
							return;
						}
						// Both RMS attribute are set: check priority
						else if ( (originRMS >= 0) && (preferredOriginRMS >= 0) && (originRMS < preferredOriginRMS) ) {
							SEISCOMP_LOG(_infoChannel, "Origin %s: RMS %.1f overrides RMS %.1f",
							             origin->publicID().c_str(), originRMS,
							             preferredOriginRMS);
							break;
						}
						// Incoming RMS is not set but preferred origin has RMS: skip incoming
						else if ( (originRMS < 0) && (preferredOriginRMS >= 0) ) {
							SEISCOMP_LOG(_infoChannel, "Origin %s has not been set preferred in event %s: RMS not set",
							             origin->publicID().c_str(), info->event->publicID().c_str());
							return;
						}
						// Incoming RMS is set but preferred origin has no RMS: prioritize incoming
						else if ( (originRMS >= 0) && (preferredOriginRMS < 0) ) {
							SEISCOMP_LOG(_infoChannel, "Origin %s: RMS %.1f overrides current unset preferred RMS",
							             origin->publicID().c_str(), originRMS);
							break;
						}
					}
					else if ( *it == "TIME" ) {
						Core::Time originCreationTime = created(origin);
						Core::Time preferredOriginCreationTime = created(info->preferredOrigin.get());
						if ( originCreationTime < preferredOriginCreationTime ) {
							SEISCOMP_DEBUG("... skipping potential preferred origin, there is a better one created later");
							return;
						}
						else if ( originCreationTime > preferredOriginCreationTime ) {
							SEISCOMP_LOG(_infoChannel, "Origin %s: %s is more recent than %s",
							             origin->publicID().c_str(), originCreationTime.iso().c_str(),
							             preferredOriginCreationTime.iso().c_str());
							break;
						}
					}
					else if ( *it == "TIME_AUTOMATIC" ) {
						EvaluationMode status = AUTOMATIC;
						try {
							status = origin->evaluationMode();
						}
						catch ( ValueException& ) {}

						// Make a noop in case of non automatic origins
						if ( status != AUTOMATIC ) continue;

						Core::Time originCreationTime = created(origin);
						Core::Time preferredOriginCreationTime = created(info->preferredOrigin.get());
						if ( originCreationTime < preferredOriginCreationTime ) {
							SEISCOMP_DEBUG("... skipping potential preferred origin, there is a better one created later");
							return;
						}
						else if ( originCreationTime > preferredOriginCreationTime ) {
							SEISCOMP_LOG(_infoChannel, "Origin %s: %s (automatic) is more recent than %s",
							             origin->publicID().c_str(), originCreationTime.iso().c_str(),
							             preferredOriginCreationTime.iso().c_str());
							break;
						}
					}
				}

				// Agency priority is a special case and an origin can become preferred without
				// a preferred magnitude if the agencyID has higher priority
				if ( !allowBadMagnitude ) {
					// The current origin has no preferred magnitude yet but
					// the event already has => ignore the origin for now
					if ( info->preferredMagnitude && !mag ) {
						SEISCOMP_DEBUG("... skipping potential preferred origin, no preferred magnitude");
						SEISCOMP_LOG(_infoChannel, "Origin %s has not been set preferred in event %s: no preferrable magnitude",
						             origin->publicID().c_str(), info->event->publicID().c_str());
						return;
					}
				}
			}
			else {
				int originAgencyPriority = agencyPriority(objectAgencyID(origin), _config);
				int preferredOriginAgencyPriority = agencyPriority(objectAgencyID(info->preferredOrigin.get()), _config);

				if ( originAgencyPriority < preferredOriginAgencyPriority ) {
					SEISCOMP_DEBUG("... skipping potential preferred origin, priority of agencyID '%s' is too low",
					               objectAgencyID(origin).c_str());
					SEISCOMP_LOG(_infoChannel, "Origin %s has not been set preferred in event %s: priority of agencyID %s is too low",
					             origin->publicID().c_str(), info->event->publicID().c_str(),
					             objectAgencyID(origin).c_str());
					return;
				}

				// Same agency priorities -> compare origin priority
				if ( originAgencyPriority == preferredOriginAgencyPriority ) {
					int originPriority = priority(origin);
					int preferredOriginPriority = priority(info->preferredOrigin.get());

					if ( info->constraints.fixOriginMode(info->preferredOrigin.get()) ) {
						SEISCOMP_LOG(_infoChannel, "Origin %s: has priority %d vs %d",
						             origin->publicID().c_str(), originPriority, preferredOriginPriority);

						// Set back the evalmode to automatic if a higher priority
						// origin has been send (but not triggered by a magnitude change only)
						if ( realOriginUpdate && (originPriority > preferredOriginPriority) ) {
							SEISCOMP_LOG(_infoChannel, "Origin %s has higher priority: releasing EvPrefOrgEvalMode",
							             origin->publicID().c_str());
							JournalEntryPtr entry = new JournalEntry;
							entry->setObjectID(info->event->publicID());
							entry->setAction("EvPrefOrgEvalMode");
							entry->setParameters("");
							entry->setSender(name() + "@" + Core::getHostname());
							entry->setCreated(Core::Time::GMT());
							Notifier::Create(_journal->publicID(), OP_ADD, entry.get());
							info->addJournalEntry(entry.get());
						}
						else
							preferredOriginPriority = ORIGIN_PRIORITY_MAX;
					}

					if ( info->constraints.fixOriginMode(origin) )
						originPriority = ORIGIN_PRIORITY_MAX;

					if ( originPriority < preferredOriginPriority ) {
						SEISCOMP_DEBUG("... skipping potential preferred origin (%d < %d)",
						               originPriority, preferredOriginPriority);
						SEISCOMP_LOG(_infoChannel, "Origin %s has not been set preferred in event %s: priority too low (%d < %d)",
						             origin->publicID().c_str(), info->event->publicID().c_str(),
						             originPriority, preferredOriginPriority);
						return;
					}

					// The current origin has no preferred magnitude yet but
					// the event already has => ignore the origin for now
					if ( info->preferredMagnitude && !mag ) {
						SEISCOMP_DEBUG("... skipping potential preferred origin, no preferred magnitude");
						SEISCOMP_LOG(_infoChannel, "Origin %s has not been set preferred in event %s: no preferrable magnitude",
						             origin->publicID().c_str(), info->event->publicID().c_str());
						return;
					}

					if ( originPriority == preferredOriginPriority ) {
						EvaluationMode status = AUTOMATIC;
						try {
							status = origin->evaluationMode();
						}
						catch ( ValueException& ) {}

						if ( status == AUTOMATIC ) {
							SEISCOMP_DEBUG("Same priority and mode is AUTOMATIC");

							if ( definingPhaseCount(origin) < definingPhaseCount(info->preferredOrigin.get()) ) {
								SEISCOMP_DEBUG("... skipping potential preferred automatic origin, phaseCount too low");
								SEISCOMP_LOG(_infoChannel, "Origin %s has not been set preferred in event %s: priorities are equal (%d) but phaseCount too low (%d < %d)",
								             origin->publicID().c_str(), info->event->publicID().c_str(), originPriority,
								             definingPhaseCount(origin), definingPhaseCount(info->preferredOrigin.get()));
								return;
							}

							/*
							if ( created(origin) < created(info->preferredOrigin.get()) ) {
								SEISCOMP_DEBUG("... skipping potential preferred origin, there is a better one created later");
								return;
							}
							*/
						}

						if ( created(origin) < created(info->preferredOrigin.get()) ) {
							SEISCOMP_DEBUG("... skipping potential preferred origin, there is a better one created later");
							return;
						}
					}
				}
				else {
					SEISCOMP_DEBUG("... agencyID '%s' overrides current agencyID '%s'",
					               objectAgencyID(origin).c_str(), objectAgencyID(info->preferredOrigin.get()).c_str());
					SEISCOMP_LOG(_infoChannel, "Origin %s: agencyID '%s' overrides agencyID '%s'",
					             origin->publicID().c_str(), objectAgencyID(origin).c_str(), objectAgencyID(info->preferredOrigin.get()).c_str());
				}
			}
		}

		info->event->setPreferredOriginID(origin->publicID());

		std::string reg = region(origin);
		EventDescription *ed = eventRegionDescription(info->event.get());
		if ( ed != NULL ) {
			if ( ed->text() != reg ) {
				ed->setText(reg);
				ed->update();
			}
		}
		else {
			EventDescriptionPtr ed = new EventDescription(reg, REGION_NAME);
			info->event->add(ed.get());
		}

		//info->event->setDescription(region(origin));
		info->preferredOrigin = origin;
		SEISCOMP_INFO("%s: set preferredOriginID to %s",
		              info->event->publicID().c_str(), origin->publicID().c_str());
		SEISCOMP_LOG(_infoChannel, "Origin %s has been set preferred in event %s",
		             origin->publicID().c_str(), info->event->publicID().c_str());

		if ( mag ) {
			if ( info->event->preferredMagnitudeID() != mag->publicID() ) {
				update = true;

				info->event->setPreferredMagnitudeID(mag->publicID());
				info->preferredMagnitude = mag;
				SEISCOMP_INFO("%s: set preferredMagnitudeID to %s",
				              info->event->publicID().c_str(), mag->publicID().c_str());
				SEISCOMP_LOG(_infoChannel, "Magnitude %s has been set preferred in event %s",
				             mag->publicID().c_str(), info->event->publicID().c_str());
			}
		}
		else {
			if ( !info->event->preferredMagnitudeID().empty() ) {
				update = true;

				info->event->setPreferredMagnitudeID("");
				info->preferredMagnitude = mag;
				SEISCOMP_INFO("%s: set preferredMagnitudeID to ''",
				              info->event->publicID().c_str());
				SEISCOMP_LOG(_infoChannel, "Set empty preferredMagnitudeID in event %s",
				             info->event->publicID().c_str());
			}
		}
	}
	else if ( mag && info->event->preferredMagnitudeID() != mag->publicID() ) {
		info->event->setPreferredMagnitudeID(mag->publicID());
		info->preferredMagnitude = mag;
		SEISCOMP_INFO("%s: set preferredMagnitudeID to %s",
		              info->event->publicID().c_str(), mag->publicID().c_str());
		SEISCOMP_INFO("%s: set preferredMagnitudeID to %s",
		              info->event->publicID().c_str(), mag->publicID().c_str());
		update = true;
	}
	else {
		SEISCOMP_INFO("%s: nothing to do", info->event->publicID().c_str());
	}

	bool callProcessors = true;

	// If only the magnitude changed, call updated processors
	if ( !update && triggeredMag && !realOriginUpdate &&
	     triggeredMag->publicID() == info->event->preferredMagnitudeID() ) {
		// Call registered processors
		EventProcessors::iterator it;
		for ( it = _processors.begin(); it != _processors.end(); ++it ) {
			if ( it->second->process(info->event.get()) )
				update = true;
		}

		// Processors have been called already
		callProcessors = false;
	}

	if ( update ) {
		if ( !info->created )
			updateEvent(info->event.get(), callProcessors);
		else {
			info->created = false;

			// Call registered processors
			EventProcessors::iterator it;
			for ( it = _processors.begin(); it != _processors.end(); ++it )
				it->second->process(info->event.get());
		}
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventTool::choosePreferred(EventInformation *info, DataModel::FocalMechanism *fm) {
	SEISCOMP_DEBUG("%s: [choose preferred focalmechanism(%s)]",
	               info->event->publicID().c_str(),
	               fm->publicID().c_str());

	bool update = false;

	if ( !info->preferredFocalMechanism ) {
		if ( isAgencyIDAllowed(objectAgencyID(fm)) || info->constraints.fixFocalMechanism(fm) ) {
			info->event->setPreferredFocalMechanismID(fm->publicID());

			info->preferredFocalMechanism = fm;
			SEISCOMP_INFO("%s: set first preferredFocalMechanismID to %s",
			              info->event->publicID().c_str(), fm->publicID().c_str());
			SEISCOMP_LOG(_infoChannel, "FocalMechanism %s has been set preferred in event %s",
			             fm->publicID().c_str(), info->event->publicID().c_str());
			update = true;
		}
		else {
			if ( !isAgencyIDAllowed(objectAgencyID(fm)) ) {
				SEISCOMP_DEBUG("%s: skip setting first preferredFocalMechanismID, agencyID '%s' is blocked",
				               info->event->publicID().c_str(), objectAgencyID(fm).c_str());
				SEISCOMP_LOG(_infoChannel, "FocalMechanism %s has not been set preferred: agencyID %s is blocked",
				             fm->publicID().c_str(), objectAgencyID(fm).c_str());
			}
			else {
				SEISCOMP_DEBUG("%s: skip setting first preferredFocalMechanismID, preferredFocalMechanismID is fixed to '%s'",
				               info->event->publicID().c_str(), info->constraints.preferredFocalMechanismID.c_str());
				SEISCOMP_LOG(_infoChannel, "FocalMechanism %s has not been set preferred: preferredFocalMechanismID is fixed to %s",
				             fm->publicID().c_str(), info->constraints.preferredFocalMechanismID.c_str());
			}
			return;
		}
	}
	else if ( info->preferredFocalMechanism->publicID() != fm->publicID() ) {
		SEISCOMP_DEBUG("... checking whether focalmechanism %s can become preferred",
		               fm->publicID().c_str());

		// Fixed focalmechanism => check if the passed focalmechanism is the fixed one
		if ( info->constraints.fixedFocalMechanism() ) {
			if ( !info->constraints.fixFocalMechanism(fm) ) {
				SEISCOMP_DEBUG("... skipping potential preferred focalmechanism, focalmechanism '%s' is fixed",
				               info->constraints.preferredFocalMechanismID.c_str());
				SEISCOMP_LOG(_infoChannel, "FocalMechanism %s has not been set preferred in event %s: focalmechanism %s is fixed",
				             fm->publicID().c_str(), info->event->publicID().c_str(),
				             info->constraints.preferredFocalMechanismID.c_str());
				return;
			}
		}
		// No fixed focalmechanism => select it using the automatic rules
		else {
			if ( isAgencyIDBlocked(objectAgencyID(fm)) ) {
				SEISCOMP_DEBUG("... skipping potential preferred focalmechanism, agencyID '%s' is blocked",
				               objectAgencyID(fm).c_str());
				SEISCOMP_LOG(_infoChannel, "FocalMechanism %s has not been set preferred in event %s: agencyID %s is blocked",
				             fm->publicID().c_str(), info->event->publicID().c_str(),
				             objectAgencyID(fm).c_str());
				return;
			}

			if ( !_config.priorities.empty() ) {
				// Run through the priority list and check the values
				for ( Config::StringList::iterator it = _config.priorities.begin();
				      it != _config.priorities.end(); ++it ) {
					if ( *it == "AGENCY" ) {
						int fmAgencyPriority = agencyPriority(objectAgencyID(fm), _config);
						int preferredFMAgencyPriority = agencyPriority(objectAgencyID(info->preferredFocalMechanism.get()), _config);

						if ( fmAgencyPriority < preferredFMAgencyPriority ) {
							SEISCOMP_DEBUG("... skipping potential preferred focalmechanism, priority of agencyID '%s' is too low",
							               objectAgencyID(fm).c_str());
							SEISCOMP_LOG(_infoChannel, "FocalMechanism %s has not been set preferred in event %s: priority of agencyID %s is too low",
							             fm->publicID().c_str(), info->event->publicID().c_str(),
							             objectAgencyID(fm).c_str());
							return;
						}
						// Found origin with higher agency priority
						else if ( fmAgencyPriority > preferredFMAgencyPriority ) {
							SEISCOMP_DEBUG("... agencyID '%s' overrides current agencyID '%s'",
							               objectAgencyID(fm).c_str(), objectAgencyID(info->preferredFocalMechanism.get()).c_str());
							SEISCOMP_LOG(_infoChannel, "FocalMechanism %s: agencyID '%s' overrides agencyID '%s'",
							             fm->publicID().c_str(), objectAgencyID(fm).c_str(), objectAgencyID(info->preferredFocalMechanism.get()).c_str());
							break;
						}
					}
					else if ( *it == "AUTHOR" ) {
						int fmAuthorPriority = authorPriority(objectAuthor(fm), _config);
						int preferredFMAuthorPriority = authorPriority(objectAuthor(info->preferredFocalMechanism.get()), _config);

						if ( fmAuthorPriority < preferredFMAuthorPriority ) {
							SEISCOMP_DEBUG("... skipping potential preferred focalmechanism, priority of author '%s' is too low",
							               objectAuthor(fm).c_str());
							SEISCOMP_LOG(_infoChannel, "FocalMechanism %s has not been set preferred in event %s: priority of author %s is too low",
							             fm->publicID().c_str(), info->event->publicID().c_str(),
							             objectAuthor(fm).c_str());
							return;
						}
						// Found focalmechanism with higher author priority
						else if ( fmAuthorPriority > preferredFMAuthorPriority ) {
							SEISCOMP_DEBUG("... author '%s' overrides current author '%s'",
							               objectAuthor(fm).c_str(), objectAuthor(info->preferredFocalMechanism.get()).c_str());
							SEISCOMP_LOG(_infoChannel, "FocalMechanism %s: author '%s' overrides author '%s'",
							             fm->publicID().c_str(), objectAuthor(fm).c_str(), objectAuthor(info->preferredFocalMechanism.get()).c_str());
							break;
						}
					}
					else if ( *it == "STATUS" ) {
						int fmPriority = priority(fm);
						int preferredFMPriority = priority(info->preferredFocalMechanism.get());

						if ( info->constraints.fixFocalMechanismMode(info->preferredFocalMechanism.get()) ) {
							SEISCOMP_LOG(_infoChannel, "FocalMechanism %s: has priority %d vs %d",
							             fm->publicID().c_str(), fmPriority, preferredFMPriority);

							// Set back the evalmode to automatic if a higher priority
							// origin has been send (but not triggered by a magnitude change only)
							if ( fmPriority > preferredFMPriority ) {
								/*
								SEISCOMP_LOG(_infoChannel, "FocalMechanism %s has higher priority: releasing EvPrefOrgEvalMode",
								             origin->publicID().c_str());
								JournalEntryPtr entry = new JournalEntry;
								entry->setObjectID(info->event->publicID());
								entry->setAction("EvPrefOrgEvalMode");
								entry->setParameters("");
								entry->setSender(name() + "@" + Core::getHostname());
								entry->setCreated(Core::Time::GMT());
								Notifier::Create(_journal->publicID(), OP_ADD, entry.get());
								info->addJournalEntry(entry.get());
								*/
							}
							else
								preferredFMPriority = FOCALMECHANISM_PRIORITY_MAX ;
						}

						if ( info->constraints.fixFocalMechanismMode(fm) )
							fmPriority = FOCALMECHANISM_PRIORITY_MAX;

						if ( fmPriority < preferredFMPriority ) {
							SEISCOMP_DEBUG("... skipping potential preferred focalmechanism (%d < %d)",
							               fmPriority, preferredFMPriority);
							SEISCOMP_LOG(_infoChannel, "FocalMechanism %s has not been set preferred in event %s: priority too low (%d < %d)",
							             fm->publicID().c_str(), info->event->publicID().c_str(),
							             fmPriority, preferredFMPriority);
							return;
						}
						// Found origin with higher status priority
						else if ( fmPriority > preferredFMPriority ) {
							SEISCOMP_LOG(_infoChannel, "FocalMechanism %s: status priority %d overrides status priority %d",
							             fm->publicID().c_str(), fmPriority, preferredFMPriority);
							break;
						}
					}
					else if ( *it == "METHOD" ) {
						int fmMethodPriority = methodPriority(fm->methodID(), _config);
						int preferredFMMethodPriority = methodPriority(info->preferredFocalMechanism->methodID(), _config);

						if ( fmMethodPriority < preferredFMMethodPriority ) {
							SEISCOMP_DEBUG("... skipping potential preferred focalMechanism, priority of method '%s' is too low",
							               fm->methodID().c_str());
							SEISCOMP_LOG(_infoChannel, "FocalMechanism %s has not been set preferred in event %s: priority of method %s is too low",
							             fm->publicID().c_str(), info->event->publicID().c_str(),
							             fm->methodID().c_str());
							return;
						}
						// Found origin with higher method priority
						else if ( fmMethodPriority > preferredFMMethodPriority ) {
							SEISCOMP_DEBUG("... methodID '%s' overrides current methodID '%s'",
							               fm->methodID().c_str(), info->preferredFocalMechanism->methodID().c_str());
							SEISCOMP_LOG(_infoChannel, "FocalMechanism %s: methodID '%s' overrides methodID '%s'",
							             fm->publicID().c_str(), fm->methodID().c_str(),
							             info->preferredFocalMechanism->methodID().c_str());
							break;
						}
					}
					else if ( *it == "TIME" ) {
						Core::Time fmCreationTime = created(fm);
						Core::Time preferredFMCreationTime = created(info->preferredFocalMechanism.get());
						if ( fmCreationTime < preferredFMCreationTime ) {
							SEISCOMP_DEBUG("... skipping potential preferred focalmechanism, there is a better one created later");
							return;
						}
						else if ( fmCreationTime > preferredFMCreationTime ) {
							SEISCOMP_LOG(_infoChannel, "FocalMechanism %s: %s is more recent than %s",
							             fm->publicID().c_str(), fmCreationTime.iso().c_str(),
							             preferredFMCreationTime.iso().c_str());
							break;
						}
					}
					else if ( *it == "TIME_AUTOMATIC" ) {
						EvaluationMode status = AUTOMATIC;
						try {
							status = fm->evaluationMode();
						}
						catch ( ValueException& ) {}

						// Make a noop in case of non automatic focalmechanisms
						if ( status != AUTOMATIC ) continue;

						Core::Time fmCreationTime = created(fm);
						Core::Time preferredFMCreationTime = created(info->preferredFocalMechanism.get());
						if ( fmCreationTime < preferredFMCreationTime ) {
							SEISCOMP_DEBUG("... skipping potential preferred focalmechanism, there is a better one created later");
							return;
						}
						else if ( fmCreationTime > preferredFMCreationTime ) {
							SEISCOMP_LOG(_infoChannel, "FocalMechanism %s: %s (automatic) is more recent than %s",
							             fm->publicID().c_str(), fmCreationTime.iso().c_str(),
							             preferredFMCreationTime.iso().c_str());
							break;
						}
					}
				}
			}
			else {
				int fmAgencyPriority = agencyPriority(objectAgencyID(fm), _config);
				int preferredFMAgencyPriority = agencyPriority(objectAgencyID(info->preferredFocalMechanism.get()), _config);

				if ( fmAgencyPriority < preferredFMAgencyPriority ) {
					SEISCOMP_DEBUG("... skipping potential preferred focalmechanism, priority of agencyID '%s' is too low",
					               objectAgencyID(fm).c_str());
					SEISCOMP_LOG(_infoChannel, "FocalMechanism %s has not been set preferred in event %s: priority of agencyID %s is too low",
					             fm->publicID().c_str(), info->event->publicID().c_str(),
					             objectAgencyID(fm).c_str());
					return;
				}

				// Same agency priorities -> compare fm priority
				if ( fmAgencyPriority == preferredFMAgencyPriority ) {
					int fmPriority = priority(fm);
					int preferredFMPriority = priority(info->preferredFocalMechanism.get());

					if ( info->constraints.fixFocalMechanismMode(info->preferredFocalMechanism.get()) ) {
						SEISCOMP_LOG(_infoChannel, "FocalMechanism %s: has priority %d vs %d",
						             fm->publicID().c_str(), fmPriority, preferredFMPriority);

						// Set back the evalmode to automatic if a higher priority
						// focalmechanism has been send
						if ( fmPriority > preferredFMPriority ) {
							/*
							SEISCOMP_LOG(_infoChannel, "Origin %s has higher priority: releasing EvPrefOrgEvalMode",
							             origin->publicID().c_str());
							JournalEntryPtr entry = new JournalEntry;
							entry->setObjectID(info->event->publicID());
							entry->setAction("EvPrefOrgEvalMode");
							entry->setParameters("");
							entry->setSender(name() + "@" + Core::getHostname());
							entry->setCreated(Core::Time::GMT());
							Notifier::Create(_journal->publicID(), OP_ADD, entry.get());
							info->addJournalEntry(entry.get());
							*/
						}
						else
							preferredFMPriority = FOCALMECHANISM_PRIORITY_MAX;
					}

					if ( info->constraints.fixFocalMechanismMode(fm) )
						fmPriority = FOCALMECHANISM_PRIORITY_MAX;

					if ( fmPriority < preferredFMPriority ) {
						SEISCOMP_DEBUG("... skipping potential preferred focalmechanism (%d < %d)",
						               fmPriority, preferredFMPriority);
						SEISCOMP_LOG(_infoChannel, "FocalMechanism %s has not been set preferred in event %s: priority too low (%d < %d)",
						             fm->publicID().c_str(), info->event->publicID().c_str(),
						             fmPriority, preferredFMPriority);
						return;
					}

					if ( fmPriority == preferredFMPriority ) {
						EvaluationMode status = AUTOMATIC;
						try {
							status = fm->evaluationMode();
						}
						catch ( ValueException& ) {}

						if ( status == AUTOMATIC ) {
							SEISCOMP_DEBUG("Same priority and mode is AUTOMATIC");

							if ( created(fm) < created(info->preferredFocalMechanism.get()) ) {
								SEISCOMP_DEBUG("... skipping potential preferred focalmechanism, there is a better one created later");
								return;
							}
						}
					}
				}
				else {
					SEISCOMP_DEBUG("... agencyID '%s' overrides current agencyID '%s'",
					               objectAgencyID(fm).c_str(), objectAgencyID(info->preferredFocalMechanism.get()).c_str());
					SEISCOMP_LOG(_infoChannel, "FocalMechanism %s: agencyID '%s' overrides agencyID '%s'",
					             fm->publicID().c_str(), objectAgencyID(fm).c_str(), objectAgencyID(info->preferredFocalMechanism.get()).c_str());
				}
			}
		}

		info->event->setPreferredFocalMechanismID(fm->publicID());

		info->preferredFocalMechanism = fm;
		SEISCOMP_INFO("%s: set preferredFocalMechanismID to %s",
		              info->event->publicID().c_str(), fm->publicID().c_str());
		SEISCOMP_LOG(_infoChannel, "FocalMechanism %s has been set preferred in event %s",
		             fm->publicID().c_str(), info->event->publicID().c_str());

		update = true;
	}
	else {
		SEISCOMP_INFO("%s: nothing to do", info->event->publicID().c_str());
	}

	if ( update ) {
		if ( !info->created )
			updateEvent(info->event.get());
		else {
			info->created = false;

			// Call registered processors
			EventProcessors::iterator it;
			for ( it = _processors.begin(); it != _processors.end(); ++it )
				it->second->process(info->event.get());
		}
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventTool::removedFromCache(Seiscomp::DataModel::PublicObject *po) {
	bool saveState = DataModel::Notifier::IsEnabled();
	DataModel::Notifier::Disable();

	EventMap::iterator it = _events.find(po->publicID());
	if ( it != _events.end() ) {
		// Remove EventInfo item for uncached event
		_events.erase(it);
		SEISCOMP_DEBUG("... removing event %s from cache", po->publicID().c_str());
	}

	po->detach();

	DataModel::Notifier::SetEnabled(saveState);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventTool::updateEvent(DataModel::Event *ev, bool callProcessors) {
	Core::Time now = Core::Time::GMT();
	// Set the modification to current time
	try {
		ev->creationInfo().setModificationTime(now);
	}
	catch ( ... ) {
		DataModel::CreationInfo ci;
		ci.setModificationTime(now);
		ev->setCreationInfo(ci);
	}

	logObject(_outputEvent, now);

	// Flag the event as updated to be sent around
	ev->update();

	if ( callProcessors ) {
		// Call registered processors
		EventProcessors::iterator it;
		for ( it = _processors.begin(); it != _processors.end(); ++it )
			it->second->process(ev);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
