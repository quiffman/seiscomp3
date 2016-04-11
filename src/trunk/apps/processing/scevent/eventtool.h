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




#ifndef __SEISCOMP_APPLICATIONS_EVENTTOOL_H__
#define __SEISCOMP_APPLICATIONS_EVENTTOOL_H__

#include <seiscomp3/client/application.h>
#include <seiscomp3/datamodel/publicobjectcache.h>
#include <seiscomp3/datamodel/eventparameters.h>
#include <seiscomp3/datamodel/journaling.h>
#include <seiscomp3/plugins/events/eventprocessor.h>

#define SEISCOMP_COMPONENT SCEVENT
#include <seiscomp3/logging/log.h>

#include "eventinfo.h"
#include "config.h"


namespace Seiscomp {

namespace DataModel {

class Pick;
class Magnitude;
class Origin;

}

namespace Client {


class EventTool : public Application {
	public:
		EventTool(int argc, char **argv);
		~EventTool();


	protected:
		void createCommandLineDescription();
		bool initConfiguration();
		bool validateParameters();

		bool init();
		bool run();

		void handleMessage(Core::Message *msg);
		void handleTimeout();

		void addObject(const std::string&, DataModel::Object* object);
		void updateObject(const std::string&, DataModel::Object* object);
		void removeObject(const std::string&, DataModel::Object* object);


	private:
		enum MatchResult {
			Nothing,
			Location,
			Picks,
			PicksAndLocation
		};

		enum DelayReason {
			SetPreferredFM
		};

		bool handleJournalEntry(DataModel::JournalEntry *);

		EventInformationPtr associateOriginCheckDelay(DataModel::Origin *);
		EventInformationPtr associateOrigin(DataModel::Origin *, bool allowEventCreation);
		void updatedOrigin(DataModel::Origin *, DataModel::Magnitude *, bool realOriginUpdate);

		EventInformationPtr associateFocalMechanismCheckDelay(DataModel::FocalMechanism *);
		EventInformationPtr associateFocalMechanism(DataModel::FocalMechanism *);
		void updatedFocalMechanism(DataModel::FocalMechanism *);

		MatchResult compare(EventInformation *info, DataModel::Origin *origin);

		EventInformationPtr createEvent(DataModel::Origin *origin);
		EventInformationPtr findMatchingEvent(DataModel::Origin *origin);
		EventInformationPtr findAssociatedEvent(DataModel::Origin *origin);
		EventInformationPtr findAssociatedEvent(DataModel::FocalMechanism *fm);

		//! Chooses the preferred origin and magnitude for an event
		void choosePreferred(EventInformation *info, DataModel::Origin *origin,
		                     DataModel::Magnitude *mag,
		                     bool realOriginUpdate = false);

		//! Chooses the preferred focal mechanism an event
		void choosePreferred(EventInformation *info, DataModel::FocalMechanism *fm);

		//! Select the preferred origin again among all associated origins
		void updatePreferredOrigin(EventInformation *info);
		void updatePreferredFocalMechanism(EventInformation *info);

		//! Merges two events. Returns false if nothing has been done due to
		//! errors. The source event
		bool mergeEvents(EventInformation *target, EventInformation *source);

		bool checkRegionFilter(const Config::RegionFilters &f, const DataModel::Origin *origin);

		//! Returns the preferred magnitude for an origin
		DataModel::Magnitude *preferredMagnitude(DataModel::Origin *origin);

		DataModel::Event *getEventForOrigin(const std::string &originID);
		DataModel::Event *getEventForFocalMechanism(const std::string &fmID);

		void cacheEvent(EventInformationPtr info);
		EventInformationPtr cachedEvent(const std::string &eventID) const;
		bool removeCachedEvent(const std::string &eventID);
		bool isEventCached(const std::string &eventID) const;

		void removedFromCache(DataModel::PublicObject *);

		void updateEvent(DataModel::Event *ev, bool = true);
		void cleanUpEventCache();

		bool hasDelayedEvent(const std::string &publicID,
		                     DelayReason reason) const;


	private:
		struct TodoEntry {
			TodoEntry(DataModel::PublicObjectPtr p,
			          DataModel::PublicObjectPtr t = NULL) : primary(p), triggered(t) {}
			DataModel::PublicObjectPtr primary;
			DataModel::PublicObjectPtr triggered;

			DataModel::PublicObject *get() const { return primary.get(); }

			DataModel::PublicObject *operator->() const {
				return primary.get();
			}

			bool operator==(const TodoEntry &other) const {
				return primary == other.primary;
			}

			bool operator<(const TodoEntry &other) const {
				return primary < other.primary;
			}
		};

		typedef DataModel::PublicObjectTimeSpanBuffer Cache;
		typedef std::map<std::string, EventInformationPtr> EventMap;
		typedef std::set<TodoEntry> TodoList;

		struct DelayedObject {
			DelayedObject(const DataModel::PublicObjectPtr &o, int t)
			: obj(o), timeout(t){}

			DataModel::PublicObjectPtr obj;
			int timeout;
		};

		struct DelayedEventUpdate {
			DelayedEventUpdate(const std::string &eid, int t, DelayReason r)
			: id(eid), timeout(t), reason(r) {}

			std::string id;
			int timeout;
			DelayReason reason;
		};

		typedef std::list<DelayedObject> DelayBuffer;
		typedef std::list<DelayedEventUpdate> DelayEventBuffer;
		typedef std::set<std::string> IDSet;

		typedef std::map<std::string, EventProcessorPtr> EventProcessors;

		double                        _fExpiry;
		Cache                         _cache;
		bool                          _testMode;
		Util::StopWatch               _timer;
		std::string                   _originID;

		Config                        _config;
		EventProcessors               _processors;

		EventMap                      _events;
		DataModel::EventParametersPtr _ep;
		DataModel::JournalingPtr      _journal;

		TodoList                      _adds;
		TodoList                      _updates;
		TodoList                      _realUpdates;
		IDSet                         _originBlackList;
		DelayBuffer                   _delayBuffer;
		DelayEventBuffer              _delayEventBuffer;

		Logging::Channel             *_infoChannel;
		Logging::Output              *_infoOutput;

		ObjectLog                    *_inputOrigin;
		ObjectLog                    *_inputMagnitude;
		ObjectLog                    *_inputFocalMechanism;
		ObjectLog                    *_inputMomentTensor;
		ObjectLog                    *_inputOriginRef;
		ObjectLog                    *_inputFMRef;
		ObjectLog                    *_inputEvent;
		ObjectLog                    *_inputJournal;
		ObjectLog                    *_outputEvent;
		ObjectLog                    *_outputOriginRef;
		ObjectLog                    *_outputFMRef;
};


}
}

#endif
