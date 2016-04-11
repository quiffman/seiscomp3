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


#define SEISCOMP_COMPONENT SCEventDump

#include <seiscomp3/logging/log.h>
#include <seiscomp3/client/application.h>
#include <seiscomp3/io/archive/xmlarchive.h>
#include <seiscomp3/datamodel/inventory.h>
#include <seiscomp3/datamodel/config.h>
#include <seiscomp3/datamodel/eventparameters.h>
#include <seiscomp3/datamodel/event.h>
#include <seiscomp3/datamodel/origin.h>
#include <seiscomp3/datamodel/pick.h>
#include <seiscomp3/datamodel/magnitude.h>
#include <seiscomp3/datamodel/stationmagnitude.h>
#include <seiscomp3/datamodel/amplitude.h>
#include <seiscomp3/datamodel/focalmechanism.h>
#include <seiscomp3/datamodel/momenttensor.h>


using namespace std;
using namespace Seiscomp;
using namespace Seiscomp::Core;
using namespace Seiscomp::IO;
using namespace Seiscomp::DataModel;


class EventDump : public Seiscomp::Client::Application {
	public:
		EventDump(int argc, char** argv) : Application(argc, argv) {
			setPrimaryMessagingGroup("LISTENER_GROUP");
			addMessagingSubscription("EVENT");
			setMessagingEnabled(true);
			setDatabaseEnabled(true, false);
			setAutoApplyNotifierEnabled(false);
			setInterpretNotifierEnabled(true);

			addMessagingSubscription("EVENT");
		}


	protected:
		void createCommandLineDescription() {
			commandline().addGroup("Dump");
			commandline().addOption("Dump", "listen", "listen to the message server for incoming events");
			commandline().addOption("Dump", "inventory,I", "export the inventory");
			commandline().addOption("Dump", "config,C", "export the config");
			commandline().addOption("Dump", "origin,O", "origin id", &_originID, false);
			commandline().addOption("Dump", "event,E", "event id", &_eventID, false);
			commandline().addOption("Dump", "with-picks,P", "export associated picks");
			commandline().addOption("Dump", "with-amplitudes,A", "export associated amplitudes");
			commandline().addOption("Dump", "with-magnitudes,M", "export station magnitudes");
			commandline().addOption("Dump", "with-focal-mechanisms,F", "export focal mechanisms");
			commandline().addOption("Dump", "ignore-arrivals,a", "do not export origin arrivals");
			commandline().addOption("Dump", "preferred-only,p", "when exporting events only the preferred origin and the preferred magnitude will be dumped");
			commandline().addOption("Dump", "all-magnitudes,m", "if only the preferred origin is exported, all magnitudes for this origin will be dumped");
			commandline().addOption("Dump", "formatted,f", "use formatted output");
			commandline().addOption("Dump", "prepend-datasize", "prepend a line with the length of the XML string");
			commandline().addOption("Dump", "output,o", "output file (default is stdout)", &_outputFile, false);
		}

		bool validateParameters() {
			if ( !commandline().hasOption("listen") ) {
				if ( !commandline().hasOption("event") && !commandline().hasOption("origin")
				     && !commandline().hasOption("inventory") && !commandline().hasOption("config") ) {
					cerr << "Require inventory flag, origin id or event id" << endl;
					return false;
				}

				setMessagingEnabled(false);
			}

			return true;
		}

		bool run() {
			if ( !query() ) {
				SEISCOMP_ERROR("No database connection");
				return false;
			}

			if ( commandline().hasOption("inventory") ) {
				InventoryPtr inv = query()->loadInventory();
				if ( !inv ) {
					SEISCOMP_ERROR("Inventory has not been found");
					return false;
				}

				XMLArchive ar;
				std::stringbuf buf;
				if ( !ar.create(&buf) ) {
					SEISCOMP_ERROR("Could not created output file '%s'", _outputFile.c_str());
					return false;
				}
	
				ar.setFormattedOutput(commandline().hasOption("formatted"));
	
				ar << inv;
				ar.close();
	
				std::string content = buf.str();
	
				if ( !_outputFile.empty() ) {
					ofstream file(_outputFile.c_str(), ios::out | ios::trunc);
	
					if ( !file.is_open() ) {
						SEISCOMP_ERROR("Could not create file: %s", _outputFile.c_str());
						return false;
					}
	
					if ( commandline().hasOption("prepend-datasize") )
						file << content.size() << endl << content;
					else
						file << content;
	
					file.close();
				}
				else {
					if ( commandline().hasOption("prepend-datasize") )
						cout << content.size() << endl << content << flush;
					else
						cout << content << flush;
					SEISCOMP_INFO("Flushing %lu bytes", (unsigned long)content.size());
				}
			}

			if ( commandline().hasOption("config") ) {
				ConfigPtr cfg = query()->loadConfig();
				if ( !cfg ) {
					SEISCOMP_ERROR("Config has not been found");
					return false;
				}

				XMLArchive ar;
				std::stringbuf buf;
				if ( !ar.create(&buf) ) {
					SEISCOMP_ERROR("Could not created output file '%s'", _outputFile.c_str());
					return false;
				}
	
				ar.setFormattedOutput(commandline().hasOption("formatted"));
	
				ar << cfg;
				ar.close();
	
				std::string content = buf.str();
	
				if ( !_outputFile.empty() ) {
					ofstream file(_outputFile.c_str(), ios::out | ios::trunc);
	
					if ( !file.is_open() ) {
						SEISCOMP_ERROR("Could not create file: %s", _outputFile.c_str());
						return false;
					}
	
					if ( commandline().hasOption("prepend-datasize") )
						file << content.size() << endl << content;
					else
						file << content;
	
					file.close();
				}
				else {
					if ( commandline().hasOption("prepend-datasize") )
						cout << content.size() << endl << content << flush;
					else
						cout << content << flush;
					SEISCOMP_INFO("Flushing %lu bytes", (unsigned long)content.size());
				}
			}

			if ( commandline().hasOption("event") ) {
				EventPtr event = Event::Cast(PublicObjectPtr(query()->getObject(Event::TypeInfo(), _eventID)));
				if ( !event ) {
					SEISCOMP_ERROR("Event with id '%s' has not been found", _eventID.c_str());
					return false;
				}

				if ( !dumpEvent(event.get()) )
					return false;
			}

			if ( commandline().hasOption("origin") ) {
				OriginPtr org = Origin::Cast(PublicObjectPtr(query()->getObject(Origin::TypeInfo(), _originID)));
				if ( !org ) {
					SEISCOMP_ERROR("Origin with id '%s' has not been found", _originID.c_str());
					return false;
				}

				if ( !dumpOrigin(org.get()) )
					return false;
			}

			if ( commandline().hasOption("listen") )
				return Application::run();

    		return true;
		}


		void addObject(const std::string& parentID,
		               Seiscomp::DataModel::Object* object) {
			updateObject(parentID, object);
		}


		void updateObject(const std::string&, Seiscomp::DataModel::Object* object) {
			Event* e = Event::Cast(object);
			if ( !e ) return;

			dumpEvent(e);
		}


		bool dumpOrigin(Origin* org) {
			SEISCOMP_INFO("Dumping Origin '%s'", org->publicID().c_str());

			EventParametersPtr ep = new EventParameters;
			ep->add(org);

			bool staMags = commandline().hasOption("with-magnitudes");
			bool ignoreArrivals = commandline().hasOption("ignore-arrivals");

			query()->load(org);

			if ( !staMags ) {
				while ( org->stationMagnitudeCount() > 0 )
					org->removeStationMagnitude(0);

				for ( size_t i = 0; i < org->magnitudeCount(); ++i ) {
					Magnitude* netMag = org->magnitude(i);
					while ( netMag->stationMagnitudeContributionCount() > 0 )
						netMag->removeStationMagnitudeContribution(0);
				}
			}

			if ( ignoreArrivals ) {
				while ( org->arrivalCount() > 0 )
					org->removeArrival(0);
			}

			if ( commandline().hasOption("with-picks") ) {
				for ( size_t a = 0; a < org->arrivalCount(); ++a ) {
					PickPtr pick = Pick::Cast(PublicObjectPtr(query()->getObject(Pick::TypeInfo(), org->arrival(a)->pickID())));
					if ( !pick ) {
						SEISCOMP_WARNING("Pick with id '%s' not found", org->arrival(a)->pickID().c_str());
						continue;
					}

					if ( !pick->eventParameters() )
						ep->add(pick.get());
				}
			}

			if ( commandline().hasOption("with-amplitudes") ) {
				for ( size_t m = 0; m < org->magnitudeCount(); ++m ) {
					Magnitude* netmag = org->magnitude(m);
					for ( size_t s = 0; s < netmag->stationMagnitudeContributionCount(); ++s ) {
						StationMagnitude* stamag = StationMagnitude::Find(netmag->stationMagnitudeContribution(s)->stationMagnitudeID());
						if ( !stamag ) {
							SEISCOMP_WARNING("StationMagnitude with id '%s' not found", netmag->stationMagnitudeContribution(s)->stationMagnitudeID().c_str());
							continue;
						}

						AmplitudePtr staamp = Amplitude::Cast(PublicObjectPtr(query()->getObject(Amplitude::TypeInfo(), stamag->amplitudeID())));

						if ( !staamp ) {
							SEISCOMP_WARNING("Amplitude with id '%s' not found", stamag->amplitudeID().c_str());
							continue;
						}

						if ( !staamp->eventParameters() )
							ep->add(staamp.get());
					}
				}
			}


			XMLArchive ar;
			std::stringbuf buf;
			if ( !ar.create(&buf) ) {
				SEISCOMP_ERROR("Could not created output file '%s'", _outputFile.c_str());
				return false;
			}

			ar.setFormattedOutput(commandline().hasOption("formatted"));

			ar << ep;
			ar.close();

			std::string content = buf.str();

			if ( !_outputFile.empty() ) {
				ofstream file(_outputFile.c_str(), ios::out | ios::trunc);

				if ( !file.is_open() ) {
					SEISCOMP_ERROR("Could not create file: %s", _outputFile.c_str());
					return false;
				}

				if ( commandline().hasOption("prepend-datasize") )
					file << content.size() << endl << content;
				else
					file << content;

				file.close();
			}
			else {
				if ( commandline().hasOption("prepend-datasize") )
					cout << content.size() << endl << content << flush;
				else
					cout << content << flush;
				SEISCOMP_INFO("Flushing %lu bytes", (unsigned long)content.size());
			}

			return true;
		}


		bool dumpEvent(Event* event) {
			SEISCOMP_INFO("Dumping Event '%s'", event->publicID().c_str());

			EventParametersPtr ep = new EventParameters;
			ep->add(event);

			bool preferredOnly = commandline().hasOption("preferred-only");
			bool staMags = commandline().hasOption("with-magnitudes");
			bool allMags = commandline().hasOption("all-magnitudes");
			bool ignoreArrivals = commandline().hasOption("ignore-arrivals");
			bool withFocMechs = commandline().hasOption("with-focal-mechanisms");

			if ( !preferredOnly )
				query()->load(event);
			else {
				query()->loadComments(event);
				query()->loadEventDescriptions(event);
				event->add(OriginReferencePtr(new OriginReference(event->preferredOriginID())).get());

				if ( withFocMechs && !event->preferredFocalMechanismID().empty() )
					event->add(FocalMechanismReferencePtr(new FocalMechanismReference(event->preferredFocalMechanismID())).get());
			}

			for ( size_t i = 0; i < event->originReferenceCount(); ++i ) {
				OriginPtr origin = Origin::Cast(PublicObjectPtr(query()->getObject(Origin::TypeInfo(), event->originReference(i)->originID())));
				if ( !origin ) {
					SEISCOMP_WARNING("Origin with id '%s' not found", event->originReference(i)->originID().c_str());
					continue;
				}

				query()->load(origin.get());

				if ( preferredOnly && !allMags ) {
					MagnitudePtr netMag;
					while ( origin->magnitudeCount() > 0 ) {
						if ( origin->magnitude(0)->publicID() == event->preferredMagnitudeID() )
							netMag = origin->magnitude(i);

						origin->removeMagnitude(0);
					}

					if ( netMag )
						origin->add(netMag.get());
				}

				if ( !staMags ) {
					while ( origin->stationMagnitudeCount() > 0 )
						origin->removeStationMagnitude(0);

					for ( size_t i = 0; i < origin->magnitudeCount(); ++i ) {
						Magnitude* netMag = origin->magnitude(i);
						while ( netMag->stationMagnitudeContributionCount() > 0 )
							netMag->removeStationMagnitudeContribution(0);
					}
				}

				if ( ignoreArrivals ) {
					while ( origin->arrivalCount() > 0 )
						origin->removeArrival(0);
				}

				ep->add(origin.get());

				if ( commandline().hasOption("with-picks") ) {
					for ( size_t a = 0; a < origin->arrivalCount(); ++a ) {
						PickPtr pick = Pick::Cast(PublicObjectPtr(query()->getObject(Pick::TypeInfo(), origin->arrival(a)->pickID())));
						if ( !pick ) {
							SEISCOMP_WARNING("Pick with id '%s' not found", origin->arrival(a)->pickID().c_str());
							continue;
						}

						if ( !pick->eventParameters() )
							ep->add(pick.get());
					}
				}

				if ( commandline().hasOption("with-amplitudes") ) {
					for ( size_t m = 0; m < origin->magnitudeCount(); ++m ) {
						Magnitude* netmag = origin->magnitude(m);
						for ( size_t s = 0; s < netmag->stationMagnitudeContributionCount(); ++s ) {
							StationMagnitude* stamag = StationMagnitude::Find(netmag->stationMagnitudeContribution(s)->stationMagnitudeID());
							if ( !stamag ) {
								SEISCOMP_WARNING("StationMagnitude with id '%s' not found", netmag->stationMagnitudeContribution(s)->stationMagnitudeID().c_str());
								continue;
							}

							AmplitudePtr staamp = Amplitude::Cast(PublicObjectPtr(query()->getObject(Amplitude::TypeInfo(), stamag->amplitudeID())));

							if ( !staamp ) {
								SEISCOMP_WARNING("Amplitude with id '%s' not found", stamag->amplitudeID().c_str());
								continue;
							}

							if ( !staamp->eventParameters() )
								ep->add(staamp.get());
						}
					}
				}
			}

			if ( !withFocMechs ) {
				while ( event->focalMechanismReferenceCount() > 0 )
					event->removeFocalMechanismReference(0);
			}

			for ( size_t i = 0; i < event->focalMechanismReferenceCount(); ++i ) {
				FocalMechanismPtr fm = FocalMechanism::Cast(PublicObjectPtr(query()->getObject(FocalMechanism::TypeInfo(), event->focalMechanismReference(i)->focalMechanismID())));
				if ( !fm ) {
					SEISCOMP_WARNING("FocalMechanism with id '%s' not found", event->focalMechanismReference(i)->focalMechanismID().c_str());
					continue;
				}

				query()->load(fm.get());
				ep->add(fm.get());

				for ( size_t m = 0; m < fm->momentTensorCount(); ++m ) {
					MomentTensor *mt = fm->momentTensor(m);
					if ( mt->derivedOriginID().empty() ) continue;

					OriginPtr derivedOrigin = ep->findOrigin(mt->derivedOriginID());
					if ( derivedOrigin != NULL ) continue;

					derivedOrigin = Origin::Cast(PublicObjectPtr(query()->getObject(Origin::TypeInfo(), mt->derivedOriginID())));
					if ( !derivedOrigin ) {
						SEISCOMP_WARNING("Derived MT origin with id '%s' not found", mt->derivedOriginID().c_str());
						continue;
					}

					query()->load(derivedOrigin.get());
					ep->add(derivedOrigin.get());
				}
			}

			XMLArchive ar;
			std::stringbuf buf;
			if ( !ar.create(&buf) ) {
				SEISCOMP_ERROR("Could not created output file '%s'", _outputFile.c_str());
				return false;
			}

			ar.setFormattedOutput(commandline().hasOption("formatted"));

			ar << ep;
			ar.close();

			std::string content = buf.str();

			if ( !_outputFile.empty() ) {
				ofstream file(_outputFile.c_str(), ios::out | ios::trunc);

				if ( !file.is_open() ) {
					SEISCOMP_ERROR("Could not create file: %s", _outputFile.c_str());
					return false;
				}

				if ( commandline().hasOption("prepend-datasize") )
					file << content.size() << endl << content;
				else
					file << content;

				file.close();
			}
			else {
				if ( commandline().hasOption("prepend-datasize") )
					cout << content.size() << endl << content << flush;
				else
					cout << content << flush;
				SEISCOMP_INFO("Flushing %lu bytes", (unsigned long)content.size());
			}

			return true;
		}


	private:
		std::string _outputFile;
		std::string _originID;
		std::string _eventID;
};


int main(int argc, char** argv) {
	EventDump app(argc, argv);
	return app.exec();
}
