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



#define SEISCOMP_COMPONENT olk

#include <seiscomp3/logging/log.h>
#include <seiscomp3/datamodel/utils.h>

#include "olk.h"
#include "locator.h"


using namespace std;
using namespace Seiscomp::Core;
using namespace Seiscomp::DataModel;
using namespace Seiscomp::Math;


namespace Seiscomp {
namespace Applications {

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
olk::olk(int argc, char** argv) : Application(argc, argv) {

	setPrimaryMessagingGroup("LISTENER_GROUP");
	addMessagingSubscription("EVENT");
	setMessagingEnabled(true);
	setDatabaseEnabled(true, false);
	setAutoApplyNotifierEnabled(false);
	setInterpretNotifierEnabled(true);
	setLoadStationsEnabled(true);
	HandleSignals(false,true);
	setMessagingUsername("");
	// Create a default filter
//	_filter = new Math::Filtering::IIR::ButterworthBandpass<float>(4, 0.7, 3.0);
 }
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

 
 // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool olk::init() {

	if (!Application::init()) return false;
	_inv = Seiscomp::Client::Inventory::Instance()->inventory();
	if ( _inv == NULL ) {
		SEISCOMP_DEBUG("Error: Could not read inventory (NULL)");
		return false;
	}
	std::cout << "Reading inventory finished" << std::endl;
	

	return true;

}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool olk::initConfiguration() {

	return Application::initConfiguration();

}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool olk::run() {

	double days=0.;
//	days=4;  																									//comment in here

	if ( !query() ) {
		SEISCOMP_ERROR("No database connection");
		return false;
	}
	std::cout << std::endl;
	std::cout << "Read how many days from DB?" << std::endl;
	std::cin >> days; 																						//comment out here
	std::cout << std::endl;
	if (readEventsFromDB(days)) menue();
	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void olk::commit(DataModel::OriginPtr originToShow) {
	for (size_t i=0; i<sentOrigins.size(); ++i) {
		if (sentOrigins[i]->publicID()==originToShow->publicID()) {
			printf("Origin with ID %s already sent\n", originToShow->publicID().c_str());
			return;
		}
	}
	DataModel::EventParameters ep;
	DataModel::NotifierMessagePtr msg = DataModel::Notifier::GetMessage();
	bool state=DataModel::Notifier::IsEnabled();
	DataModel::Notifier::Enable();
	ep.add(originToShow.get());
	DataModel::Notifier::SetEnabled(state);
	msg = DataModel::Notifier::GetMessage(true);
	if(msg) connection()->send("LOCATION",msg.get());
	printf("Sending Origin with ID %s\n", originToShow->publicID().c_str());
	sentOrigins.push_back(originToShow);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void olk::loadPicksnAmplitudes(const std::string& originID) {

	if (query()) {
		DatabaseIterator it = query()-> getPicks(originID);
		for ( ; *it; ++it ) {
			DataModel::PickPtr pick = Pick::Cast(*it);
			if ( pick ) {
				_picks.push_back(pick.get());
				DataModel::AmplitudePtr amplitude; amplitude=query()->getAmplitude(pick->publicID(),"mb");
				_amplitudes.push_back(amplitude);
			}
		}
	}

}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool olk::readEventsFromDB(double days)
{
	bool foundAgency=false;
	std::vector<DataModel::OriginPtr> tempOrigins;

	if (!query())
		return false;
	if (days <= 0)
		return false;
	
	std::cout << "Reading events from database" << std::endl;
	
	long int numberOfDays = static_cast<long int>(floor(days * 86400));
	printf("Looking for events in the past %f (%li sec.)\n", days, numberOfDays);
	std::cout << std::endl;
	Core::Time endTime = Core::Time::GMT(); // Today
	Core::Time startTime = endTime - Core::Time::TimeSpan(numberOfDays, 0);

	for (DataModel::DatabaseIterator ite = query()->getEvents(startTime, endTime); *ite != NULL; ++ite) {
		DataModel::EventPtr event=DataModel::Event::Cast(*ite);
		if (event) {
			_events.push_back(event);
		}
	}
	printf("%i Events\n", int(_events.size()));
	for (size_t i=0; i<_events.size(); ++i) {
		tempOrigins.clear();
		for (DataModel::DatabaseIterator ito = query()->getOrigins(_events[i]->publicID()); *ito != NULL; ++ito) {	
			DataModel::OriginPtr origin=DataModel::Origin::Cast(*ito);
			if (!origin) {
				std::cout << "No Origin" << std::endl;
				continue;
			}
			foundAgency=false;
			for (std::vector<DataModel::OriginPtr>::iterator tempit=tempOrigins.begin(); tempit!=tempOrigins.end(); ++tempit) {
				bool equalAgencies;
				try {
					equalAgencies = origin->creationInfo().agencyID()==(*tempit)->creationInfo().agencyID();
				}
				catch ( Core::ValueException & ) {
					// Maybe true?
					equalAgencies = false;
				}

				if (equalAgencies) {
					if (origin->publicID()==_events[i]->preferredOriginID()) {
						tempOrigins.erase(tempit);
						tempOrigins.insert(tempit,1,origin);
					}
					else {
						try {
							if (origin->creationInfo().creationTime()>(*tempit)->creationInfo().creationTime() &&
							    (*tempit)->publicID()!=_events[i]->preferredOriginID()) {
								tempOrigins.erase(tempit);
								tempOrigins.insert(tempit,1,origin);
							}
						}
						catch ( Core::ValueException & ) {}
					}
					foundAgency=true;	
				}
			}
			if (!foundAgency) {
				tempOrigins.push_back(origin);
			}
		}		
		for (size_t k=0; k<tempOrigins.size();++k) {
			query()->load(tempOrigins[k].get());	
			_origins.push_back(tempOrigins[k]);
			_events[i]->add(new OriginReference(tempOrigins[k]->publicID()));
		}
	}
	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void olk::plotEvents() {
	int counter=1;

	for (std::vector<DataModel::EventPtr>::iterator eit = _events.begin(); eit != _events.end(); ++eit) {	
		bool found=false;
		for (std::vector<DataModel::OriginPtr>::iterator oit = _origins.begin(); oit != _origins.end(); ++oit) {
			if ((*eit)->preferredOriginID()	== (*oit)->publicID()) {
				_netMag = (*oit)->findMagnitude((*eit)->preferredMagnitudeID());
				found=true;
				if (_netMag) {
					printf("%3i) %20s %3.1f %-6.6s %3i %9.4f %9.4f %5.1f %9.9s %2i %-20.20s %5s %11.11s\n", 
						counter,
						(*oit)->time().value().toString("%Y-%m-%d %H:%M:%S").c_str(),
						_netMag->magnitude().value(),
						_netMag->type().c_str(),
						int((*oit)->arrivalCount()),
						(*oit)->latitude().value(), 
						(*oit)->longitude().value(), 
						(*oit)->depth().value(),
						(*oit)->evaluationMode().toString(),
						int((*eit)->originReferenceCount()),
						eventRegion(eit->get()).c_str(),
						objectAgencyID(eit->get()).c_str(),
						(*eit)->publicID().c_str()
					);
				}
				else {
					printf("%3i) %20s %-12.12s %3i %9.4f %9.4f %5.1f %9.9s %2i %-20.20s %5s %11.11s\n", 
						counter,
						(*oit)->time().value().toString("%Y-%m-%d %H:%M:%S").c_str(),
						"no magnitude",
						int((*oit)->arrivalCount()),
						(*oit)->latitude().value(), 
						(*oit)->longitude().value(), 
						(*oit)->depth().value(),
						(*oit)->evaluationMode().toString(),
						int((*eit)->originReferenceCount()),
						eventRegion(eit->get()).c_str(),
						objectAgencyID(eit->get()).c_str(),
						(*eit)->publicID().c_str()
					);
				}
				break;
			}
		}
		counter=counter+1;
		if (!found) std::cout << "Origin not found" << std::endl;
	}

}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void olk::plotEventInformation() {
	DataModel::EventPtr tmpEvent=_events[_actualEvent];
	
	std::cout << "# Event information:" << std::endl;
	printf("EID: %.40s\n", tmpEvent->publicID().c_str());
	printf("ECT: %.20s\n", tmpEvent->creationInfo().creationTime().toString("%Y-%m-%d %H:%M:%S").c_str());
	try {
		printf("EPO: %40s\n", tmpEvent->preferredOriginID().c_str());
	}
	catch ( Core::GeneralException& e ) {
		printf("EPO: Preferred origin not set\n");
	}
	try {
		printf("EPM: %.40s\n", tmpEvent->preferredMagnitudeID().c_str());
	}
	catch ( Core::GeneralException& e ) {
		printf("EPM: Preferred magnitude not set\n");
	}
	try {
		printf("EVD: %.40s\n", eventRegion(tmpEvent.get()).c_str());
	}
	catch ( Core::GeneralException& e ) {
		printf("EVD: Event description not set\n");
	}
	try {
		printf("EVT: %.10s\n", tmpEvent->type().toString());
	}
	catch ( Core::GeneralException& e ) {
		printf("EVT: Event type not set");
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void olk::plotOriginInformation(DataModel::OriginPtr originToShow, bool shortInfo) {
//	DataModel::EventPtr tmpEvent=_events[_actualEvent];

	if (shortInfo) {
		std::cout << "# origin information:" << std::endl;
		printf("%.40s %11.6f %11.6f %5.1f %3i\n", originToShow->publicID().c_str(), originToShow->latitude().value(), originToShow->longitude().value(), originToShow->depth().value(), int(originToShow->arrivalCount()));
	}
	else {
		std::cout << "# origin information:" << std::endl;
		printf("OID: %.40s\n", originToShow->publicID().c_str());
		printf("OCT: %s\n", originToShow->creationInfo().creationTime().toString("%Y-%m-%d %H:%M:%S").c_str());
		printf("OST: %s\n", originToShow->time().value().toString("%Y-%m-%d %H:%M:%S").c_str());
		printf("OLA: %11.6f\n", originToShow->latitude().value());
		printf("OLO: %11.6f\n", originToShow->longitude().value());
		try {
			printf("ODE: %6f\n", originToShow->depth().value());
		}
		catch ( Core::GeneralException& e ) {
			printf("ODE: Depth not set\n");
		}
		printf("OST: %s\n", originToShow->evaluationMode().toString());
		int activeArrivals=0;
		for (size_t i=0; i < originToShow->arrivalCount();++i) {
			if (originToShow->arrival(i)->weight()>0) activeArrivals=activeArrivals+1;
		}
		printf("OAC: %4i (%4i)\n", activeArrivals, int(originToShow->arrivalCount()));
		for (size_t i=0; i < originToShow->arrivalCount();++i) {
			for (size_t j=0; j < _picks.size(); ++j) {
				if (_picks[j]->publicID()== originToShow->arrival(i)->pickID()) {
					printf("PHA: %2s.%5s.%2s.%3s %s %5.5s %s %5.2f %6.2f %6.2f %3.1f %s\n",
						_picks[j]->waveformID().networkCode().c_str(),
						_picks[j]->waveformID().stationCode().c_str(),
						_picks[j]->waveformID().locationCode().c_str(),
						_picks[j]->waveformID().channelCode().c_str(),
						_picks[j]->time().value().toString("%Y-%m-%d %H:%M:%S").c_str(),
						originToShow->arrival(i)->phase().code().c_str(),
						_picks[j]->evaluationMode().toString(),
						originToShow->arrival(i)->timeResidual(),
						originToShow->arrival(i)->distance(),
						originToShow->arrival(i)->azimuth(),
						originToShow->arrival(i)->weight(),
						_picks[j]->creationInfo().creationTime().toString("%Y-%m-%d %H:%M:%S").c_str()
						);
				}
			}
		}
	
		for (size_t i=0; i<originToShow->magnitudeCount(); ++i) {
			if (i==0) std::cout << "# magnitude information:" << std::endl;
			std::cout << "NMG: " << originToShow->magnitude(i)->publicID() << std::endl;
			std::cout << "NMI: " << originToShow->magnitude(i)->type() << " " << originToShow->magnitude(i)->magnitude().value() << " " << originToShow->magnitude(i)->stationMagnitudeContributionCount() << " " << 
			originToShow->magnitude(i)->creationInfo().creationTime().toString("%Y-%m-%d %H:%M:%S") << std::endl;
			for (size_t j=0; j < originToShow->magnitude(i)->stationMagnitudeContributionCount(); ++j) {
				StationMagnitude* stamag = StationMagnitude::Find(originToShow->magnitude(i)->stationMagnitudeContribution(j)->stationMagnitudeID());
				if (stamag) {
					printf("SMI: %2s.%5s.%2s.%3s %3.1f %10.10s %s\n", 
					_picks[j]->waveformID().networkCode().c_str(),
					_picks[j]->waveformID().stationCode().c_str(),
					_picks[j]->waveformID().locationCode().c_str(),
					_picks[j]->waveformID().channelCode().c_str(),
					stamag->magnitude().value(),
					stamag->type().c_str(),
					stamag->creationInfo().creationTime().toString("%Y-%m-%d %H:%M:%S").c_str()
					);
				}
			}
		}
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void olk::plotResiduals(DataModel::OriginPtr originToShow) {
	std::vector<double> xvalues;
	std::vector<double> yvalues;
	std::vector<double> weight;

	std::cout << std::endl;
	printf("Arrival count: %4i RMS: %5.2f\n", int(originToShow->arrivalCount()), originToShow->quality().standardError());
	std::cout << std::endl;
	for (size_t i=0; i<originToShow->arrivalCount(); ++i) {
		xvalues.push_back(originToShow->arrival(i)->azimuth());
		yvalues.push_back(originToShow->arrival(i)->timeResidual());
		weight.push_back(originToShow->arrival(i)->weight());
	}	
	std::cout << "x= Azimuth (°), y= Absolute residual (s)" << std::endl;	
	plotDiagram(xvalues,yvalues,weight);
	xvalues.clear();
	yvalues.clear();
	weight.clear();
	for (size_t i=0; i<originToShow->arrivalCount(); ++i) {
		xvalues.push_back(originToShow->arrival(i)->distance());
		yvalues.push_back(originToShow->arrival(i)->timeResidual());
		weight.push_back(originToShow->arrival(i)->weight());
	}		
	std::cout << std::endl;
	std::cout << "x= Distance (°), y= Absolute residual (s)" << std::endl;	
	std::cout << std::endl;
	plotDiagram(xvalues,yvalues,weight);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void olk::plotMagnitudes(DataModel::OriginPtr originToShow, int cmdMagType) {
	std::vector<double> xvalues;
	std::vector<double> yvalues;	
	std::vector<double> weight;		
		
	for (size_t i=0; i<originToShow->magnitude(cmdMagType)->stationMagnitudeContributionCount();++i) {
		DataModel::StationMagnitudePtr staMag= originToShow->findStationMagnitude(originToShow->magnitude(cmdMagType)->stationMagnitudeContribution(i)->stationMagnitudeID());
		for (size_t j=0; j<_picks.size(); ++j) {
			if (_picks[j]->waveformID()==staMag->waveformID()) {
				for (size_t k=0; k<originToShow->arrivalCount(); ++k) {
					if (_picks[j]->publicID()==originToShow->arrival(k)->pickID()) {
						xvalues.push_back(originToShow->arrival(k)->distance());
						yvalues.push_back(staMag->magnitude().value()-originToShow->magnitude(cmdMagType)->magnitude().value());
						weight.push_back(originToShow->magnitude(cmdMagType)->stationMagnitudeContribution(i)->weight());
					}
				}
			}
		}
	}
	std::cout << std::endl;
	std::cout << std::endl;
	std::cout << "x = Distance (°), y = Absolute stationmagnitude scatter" << std::endl;
	plotDiagram(xvalues,yvalues,weight);
	std::cout << std::endl;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void olk::deselectArrivalByDistance(DataModel::OriginPtr tempOrigin) {
	double treshold;
//	treshold=15;																								//comment in here

	std::cout << "Treshold to exclude outliers (degree)" << std::endl;
	std::cin >> treshold;																					//comment out here
	std::cout << std::endl;
	for (size_t i=0; i<tempOrigin->arrivalCount(); ++i) {
		if (tempOrigin->arrival(i)->distance() > treshold) {
			tempOrigin->arrival(i)->setWeight(0.0);
		}
	}
	std::cout << "acount" << tempOrigin->arrivalCount() << std::endl;
	plotResiduals(tempOrigin);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void olk::deselectArrivalByResidual(DataModel::OriginPtr tempOrigin) {
	double utreshold;
	double ltreshold;
	std::cout << "Upper treshold to exclude outliers (residual)" << std::endl;
	std::cin >> utreshold;
	std::cout << "Lower treshold to exclude outliers (residual)" << std::endl;
	std::cin >> ltreshold;
	std::cout << std::endl;
	for (size_t i=0; i<tempOrigin->arrivalCount(); ++i) {
		if (tempOrigin->arrival(i)->timeResidual() > utreshold || (tempOrigin->arrival(i)->timeResidual() < ltreshold)) {
			tempOrigin->arrival(i)->setWeight(0.0);
		}
	}
	plotResiduals(tempOrigin);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void olk::selectAllArrivals(DataModel::OriginPtr tempOrigin) {
	for (size_t i=0; i<tempOrigin->arrivalCount(); ++i) {
		tempOrigin->arrival(i)->setWeight(1.0);
	}
	plotResiduals(tempOrigin);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void olk::plotDiagram(std::vector<double> xvalues, std::vector<double> yvalues, std::vector<double> weight) {

	double ly;
	double uy;
	double lx;
	double ux;
	double maxX=getMax(xvalues);
	double maxY=getMax(yvalues);
	double xSpacing;
	double ySpacing;
	double label;
	int count;
	int lastlabel;
	int symbol=-1;

	lastlabel=-10;
	xSpacing=(int(maxX/5.0)+1)*5.0/50.0;
	ySpacing=(int(maxY/1.0)+1)*1.0/10.0;

	std::cout << "+ = selected,   # = several selected" << std::endl;
	std::cout << "- = deselected, = = several deselected" << std::endl;	
	std::cout << "* = selected and deselected" << std::endl;	
	std::cout << std::endl;

	for (size_t y=20; y>0; --y) {

		ly=(double(y)-11.)*ySpacing;
		uy=(double(y)-11.+1.)*ySpacing;
		label=uy;
		printf("%+5.1f|", uy);
		/*if (fmod(label,1)==0) {
			std::cout << label;
			if (label <10) std::cout << " |";
			else if ((label>=10) && (label<100)) std::cout << "|";			
		}
		else std::cout << "  |";*/
		for (size_t x=0; x<50; ++x) {
			lx=x*xSpacing;
			ux=(x+1)*xSpacing;
			count=0;
			for (size_t i=0; i<yvalues.size(); ++i) {
				if ((yvalues[i] >= ly) && (yvalues[i] < uy) && (xvalues[i] >= lx) && (xvalues[i] < ux)) {
					if (weight[i]==0.0) {
						if (symbol==-1) symbol=0;
						else if (symbol==1) symbol=2;						
					}
					else {
						if (symbol==-1) symbol=1;	
						else if (symbol==0) symbol=2;
					}
					count=count+1;
				}
			}
			if (count==0) {
				std::cout << " ";
			}
			else if (count==1) {
				if (symbol==1) {
					std::cout << "+";
				}
				else if (symbol==0) {
					std::cout << "-";
				}
			}
			else if (count>1) {
				if (symbol==1) {
					std::cout << "#";
				}
				else if (symbol==0) {
					std::cout << "=";
				}
				else if (symbol==2) {
					std::cout << "*";
				}
			}
			symbol=-1;
		}
		std::cout << std::endl;	
	}
	std::cout << "    ";
	for (size_t i=0;i<=50; ++i) std::cout << "―";
	std::cout << std::endl;
	std::cout << "    ";
	for (int i=0;i<=50; ++i) {
		label=i*xSpacing;
		if ((fmod(label,1)==0) && (i-lastlabel)>3) {
			std::cout << label;
			lastlabel=i;
			if ((label>=10) && (label<100)) i=i+1;
			else if ((label>=100) && (label<1000)) i=i+2;
		}
		else std::cout << " ";
	}
	std::cout << std::endl;
	std::cout << std::endl;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
double olk::getMax(std::vector<double> values) {

	double max=0;

	for (size_t i=0; i<values.size(); ++i) {

		if (abs(values[i]>max)) max=values[i];

	}
	return max;

}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<	


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool olk::reLocate(DataModel::OriginPtr tempOrigin, std::string depth) {
	std::string cmdRelocate;
	Locator locator;

	try {
		if (atoi(depth.c_str())>=0) {
			locator.setLocatorParams(LP_FIX_DEPTH, "y");
			locator.setLocatorParams(LP_FIXING_DEPTH, depth.c_str());
		}
		_originInRevision = locator.relocate(tempOrigin.get());
		_originInRevision->setEvaluationMode(EvaluationMode(MANUAL));
		if ( !_originInRevision ) {
			std::cout << "The relocation failed for some reason." << std::endl;
			return false;
		}
	}
	catch ( Core::GeneralException& e ) {
		std::cout << "Relocation error" << std::endl;
		return false;
	}
	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
DataModel::OriginPtr olk::copyOrigin(DataModel::OriginPtr sourceOrigin) {
	DataModel::OriginPtr targetOrigin= DataModel::Origin::Create();
	targetOrigin = sourceOrigin;
	for (size_t i=0; i < sourceOrigin->arrivalCount(); ++i) {
		DataModel::ArrivalPtr targetArrival;
		targetArrival=sourceOrigin->arrival(i);
		targetOrigin->add(targetArrival.get());
	}
	for (size_t i=0; i < sourceOrigin->magnitudeCount(); ++i) {
		DataModel::MagnitudePtr targetNetMag=DataModel::Magnitude::Create();
		targetNetMag=sourceOrigin->magnitude(i);
		targetOrigin->add(targetNetMag.get());
		for (size_t j=0; j < sourceOrigin->magnitude(i)->stationMagnitudeContributionCount(); ++j) {
			DataModel::StationMagnitudeContributionPtr targetMagRef;
			targetMagRef=sourceOrigin->magnitude(i)->stationMagnitudeContribution(j);
			targetNetMag->add(targetMagRef.get());
		}
	}
	for (size_t i=0; i < sourceOrigin->stationMagnitudeCount(); ++i) {
		DataModel::StationMagnitudePtr targetStaMag=DataModel::StationMagnitude::Create();
		targetStaMag=sourceOrigin->stationMagnitude(i);
		targetOrigin->add(targetStaMag.get());
	}
	return targetOrigin;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void olk::menue() {

	std::string magTypeSelection="";
	std::string preferredString="*";


	std::string eventSelection="";
	while (eventSelection!="q" && !_exitRequested) {
		plotEvents();
		std::cout << std::endl;
		std::cout << "Choose event or quit (q)" << std::endl;
		std::cin >> eventSelection;
		std::cout << std::endl;
		_actualEvent=atoi(eventSelection.c_str())-1;
		if ((_actualEvent+1>_events.size()) || (_actualEvent<0)) continue;
		if (eventSelection=="q") break;
		int counter=1;
		for (size_t i=0; i<_events[_actualEvent]->originReferenceCount(); ++i) {
			for (size_t j=0; _origins.size(); ++j) {
				if (_events[_actualEvent]->originReference(i)->originID()==_origins[j]->publicID()) {
					if (_events[_actualEvent]->preferredOriginID()==_events[_actualEvent]->originReference(i)->originID()) {
						preferredString="*";
					}
					else {
						preferredString=" ";
					}
					printf("%i %s) %-40.40s %20s %11.6f %11.6f %6.2f %5s\n",
						counter,
						preferredString.c_str(),
						_origins[j]->publicID().c_str(),
						_origins[j]->time().value().toString("%Y-%m-%d %H:%M:%S").c_str(),
						_origins[j]->latitude().value(),
						_origins[j]->longitude().value(),
						_origins[j]->depth().value(),
						objectAgencyID(_origins[j].get()).c_str()
					);	
					counter=counter+1;
					break;
				}
			}
		}
		int originCount=0;
		std::string chooseOrigin="";
		std::cout << std::endl;
		while (chooseOrigin!="b" && !_exitRequested) {
			std::cout << "Choose Origin (* preferred) or go back (b)" << std::endl;
			std::cin >> chooseOrigin;
			originCount=atoi(chooseOrigin.c_str());
			if (chooseOrigin=="b") break;
			if ((originCount>=counter) || (originCount<=0)) continue;
			std::cout << originCount << std::endl;
			for (size_t i=0; i<_origins.size();++i) {
				if (_events[_actualEvent]->originReference(originCount-1)->originID()==_origins[i]->publicID()) 
					_originInRevision = copyOrigin(_origins[i]);
			}
			loadPicksnAmplitudes(_originInRevision->publicID());
			plotOriginInformation(_originInRevision,false);
			std::string revTypeSelection="";		
			while (revTypeSelection!="b" && !_exitRequested) {
				std::cout << "Review location (l), magnitudes (m), commit (c) or go back (b)" << std::endl;
				std::cin >> revTypeSelection;
				std::cout << std::endl;
				if (revTypeSelection=="b") break;
				else if (revTypeSelection=="l") {
					std::string phaseSelection="";
					while (phaseSelection!="b" && !_exitRequested) {
						//std::cout << "Revise arrivals (a), fix depth (d), relocate (l) or go back (b)" << 	std::endl;
						std::cout << "Revise arrivals (a), relocate (l) or go back (b)" << 	std::endl;
						std::cin >> phaseSelection;
						std::cout << std::endl;
						if (phaseSelection=="b") break;
						else if (phaseSelection=="a") {						
							std::string locAttributeSelection="";
							plotResiduals(_originInRevision);
							while (locAttributeSelection!="b" && !_exitRequested) {
								std::cout << "Revise be distance (d), residual (r), select all (a) or go back (b)" << std::endl;
								std::cin >> locAttributeSelection;
								std::cout << std::endl;
								if (locAttributeSelection=="b") break;
								else if (locAttributeSelection=="d") {
										deselectArrivalByDistance(_originInRevision);
								}
								else if (locAttributeSelection=="r") {
									deselectArrivalByResidual(_originInRevision);
								}
								else if (locAttributeSelection=="a") {
									selectAllArrivals(_originInRevision);
								}
							}
						}
/*						else if (phaseSelection=="d") {	
							plotResiduals(_originInRevision);
							fixDepth(_originInRevision);
							plotResiduals(_originInRevision);
							plotOriginInformation(_originInRevision,true);
						}*/
						else if (phaseSelection=="l") {
							reLocate(_originInRevision,"");
							plotResiduals(_originInRevision);
							plotOriginInformation(_originInRevision,true);
						}	
					}	
				}	
				else if (revTypeSelection=="m") {
					listMagTypes(_originInRevision);
					std::cout << "Choose magnitude type or go back (b)" << std::endl;
					std::cin >> magTypeSelection;
					std::cout << std::endl;
					int magNumber=atoi(magTypeSelection.c_str())-1;				
					if ((magNumber<0)||(magNumber>=_originInRevision->magnitudeCount())) {
						continue;
					}
	
					plotMagnitudes(_originInRevision, magNumber);
					std::string MagAttributeSelection="";
	/*				while (MagAttributeSelection!="b" && !_exitRequested) {
						std::cout << "Revise by distance (d), residual (r), recalculate (c) or go back (b)"  << std::endl;	
						std::cin >> MagAttributeSelection;
						std::cout << std::endl;
						if (MagAttributeSelection=="b") break;
						else if (MagAttributeSelection=="d") {	
							se
						}
						else if (MagAttributeSelection=="r") {	
						}
						else if (MagAttributeSelection=="c") {	
						}				
					}*/
				}
				else if (revTypeSelection=="c") {
					commit(_originInRevision);
				}
			}
		}

	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool olk::fixDepth(DataModel::OriginPtr tempOrigin) {
	std::string depth="";
	std::cout << "Fix depth to or go back (b)" << std::endl;
	std::cin >> depth;
	if (atoi(depth.c_str())<=0) return false;
	reLocate(tempOrigin,depth);
	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void olk::listMagTypes(DataModel::OriginPtr originToShow) {
	std::string preferredString;
	DataModel::MagnitudePtr preNetMag = originToShow->findMagnitude(_events[_actualEvent]->preferredMagnitudeID());
	if (!preNetMag) std::cout << "preferred NetMag not found" << std::endl;
	for (size_t i=0; i<originToShow->magnitudeCount(); ++i) {
		if (originToShow->magnitude(i)->type()==preNetMag->type()) preferredString="*";
		else preferredString=" ";
		printf("%i %s) %s\n", int(i+1), preferredString.c_str() , originToShow->magnitude(i)->type().c_str());
	}
	std::cout << std::endl; 
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


/*// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void olk::pick(DataModel::OriginPtr originToShow) {
	const char* service = "arclink";
	const char* dataSource = "st11:18001";
	double minDistance=180.0;
	double maxDistance=0.0;	
	std::string minPickID;
	std::string maxPickID;
	Time startTime;
	Time endTime;
	
	for (size_t i=0; i < _originInRevision->arrivalCount();++i) {
		if (_originInRevision->arrival(i)->distance()>maxDistance) {
			maxDistance=_originInRevision->arrival(i)->distance();
			maxPickID=_originInRevision->arrival(i)->pickID();
		}
		if (_originInRevision->arrival(i)->distance()<minDistance) {
			minDistance=_originInRevision->arrival(i)->distance();
			minPickID=_originInRevision->arrival(i)->pickID();
		}
	}		
	for (size_t i=0; i < _picks.size(); ++i) {	
		if (minPickID==_picks[i]->publicID()) startTime=_picks[i]->time();
		if (maxPickID==_picks[i]->publicID()) endTime=_picks[i]->time();
	}

	IO::RecordStreamPtr rec_stream = IO::RecordStream::Create(service);
	if ( rec_stream == NULL ) {
		printf("could not create stream for service '%s'\n", service);
	}

	if ( !rec_stream->setSource(dataSource) ) {
		printf("setting datasource '%s' failed\n", dataSource);
	}
	// Half an hour ringbuffer
	_recordBuffer.setTimeWindow(arrivalTimeSpan(originToShow);
	

	// Add stream here

	for (size_t i=0; i < _originInRevision->arrivalCount();++i) {
		for (size_t j=0; j < _picks.size(); ++j) {
			if (_picks[j]->publicID()== _originInRevision->arrival(i)->pickID()) {
				rec_stream->addStream(_picks[j]->waveformID().networkCode(), _picks[j]->waveformID().stationCode(), _picks[j]->waveformID().locationCode(), _picks[j]->waveformID().channelCode());
				// Set the starttime for the request
				rec_stream->setStartTime(_picks[j]->time().value() - TimeSpan(20.0));
				// Set the endtime for the request
				rec_stream->setEndTime(_picks[j]->time().value() - TimeSpan(40.0));
				Util::StopWatch timer;
				IO::RecordInput rec_input(rec_stream.get(), Array::DOUBLE, Record::DATA_ONLY);
			}
		}
	}
	IO::RecordInput rec_input(rec_stream.get(), Array::DOUBLE, Record::DATA_ONLY);

	try {
		rec_stream->setTimeout(10);

		for ( IO::RecordIterator it = rec_input.begin(); it != rec_input.end(); ++it ) {
			// Hold a SmartPointer to the fetched record to avoid memory leaks
			RecordCPtr rawRec = *it;
			RecordCPtr rec = filteredRecord(rawRec.get());
			recordBuffer.feed(rec.get());
		}
	}
	catch ( IO::RecordStreamTimeout ) {
		printf("timeout\n");
	}
	catch ( Core::OperationInterrupted ) {
		printf("interrupted\n");
	}

}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Record* olk::filteredRecord(const Record* rec) const {
	FloatArrayPtr arr = (FloatArray*)rec->data()->copy(Array::FLOAT);

	GenericRecord* crec = new GenericRecord(*rec);
	_filter->apply(arr->size(), arr->typedData());

	try {
		crec->endTime();
	}
	catch (...) {
		SEISCOMP_ERROR("Filtered record has invalid endtime -> skipping");
		delete crec;
		return NULL;
	}

	crec->setData(arr.get());

	return crec;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Core::TimeWindow olk::arrivalTimeSpan(DataModel::OriginPtr tempOrigin) {
	Time startTime=Time::GMT();
	Time endTime;
	for (size_t i=0; i < _originInRevision->arrivalCount();++i) {
		for (size_t j=0; j < _picks.size(); ++j) {
			if (_picks[j]->publicID()== _originInRevision->arrival(i)->pickID()) {
				if (startTime>_picks[j]->time().value()) {
					startTime=_picks[j]->time().value();
				}
				if (endTime<_picks[j]->time().value()) {
					endTime=_picks[j]->time().value();
				}
			}
		}
	}
	Core::TimeWindow myTimeWindow(startTime-Core::TimeSpan(30.0),endTime+Core::TimeSpan(60.0));
	return myTimeWindow;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
plotWaveforms(DataModel::OriginPtr originToShow) {
	for (size_t i; i<originToShow->arrivalCount();++i) {
		
	}
}*/

}
}
