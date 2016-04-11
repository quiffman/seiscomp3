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



#define SEISCOMP_COMPONENT LocSAT
#include <seiscomp3/logging/log.h>
#include <seiscomp3/core/strings.h>
#include <seiscomp3/config/environment.h>
#include <seiscomp3/seismology/locsat.h>
#include <seiscomp3/seismology/ttt.h>
#include <seiscomp3/math/geo.h>
#include "locsat_internal.h"
#include <stdlib.h>
#include <math.h>
#include <list>
#include <algorithm> 

// #define LOCSAT_TESTING

using namespace Seiscomp::DataModel;
using namespace Seiscomp::Seismology;


namespace Seiscomp{


std::string LocSAT::_defaultTablePrefix = "iasp91";
REGISTER_LOCATOR(LocSAT, "LOCSAT");


LocSAT::~LocSAT(){
	delete _locator_params;
	if (_locateEvent)
		delete _locateEvent;
}

LocSAT::LocSAT() {
	_name = "LOCSAT";
	_newOriginID = "";

	_locator_params = new Internal::Locator_params;
	_locator_params->outfile_name = new char[1024];
	_locator_params->prefix = new char[1024];

	_locateEvent = NULL;
	_profiles.push_back("iasp91");
	_profiles.push_back("tab");
	setProfile(_defaultTablePrefix);
	setDefaultLocatorParams();
}


bool LocSAT::init(const Config &config) {
	try {
		_profiles = config.getStrings("LOCSAT.profiles");
	}
	catch ( ... ) {
		_profiles.clear();
		_profiles.push_back("iasp91");
		_profiles.push_back("tab");
	}

	return true;
}

void LocSAT::setNewOriginID(const std::string& newOriginID) {
	_newOriginID = newOriginID;
}


int LocSAT::capabilities() const {
	return InitialLocation | FixedDepth;
}


DataModel::Origin* LocSAT::locate(PickList& pickList,
                                  double initLat, double initLon, double initDepth,
                                  Seiscomp::Core::Time& initTime) throw(Core::GeneralException) {
	if (_locateEvent) delete _locateEvent;
	_locateEvent = new Internal::LocSAT;
	_locateEvent->setOrigin(initLat, initLon, initDepth);
	_locateEvent->setOriginTime((double)initTime);

	setLocatorParams(LP_USE_LOCATION, "TRUE");

	return fromPicks(pickList);
}


DataModel::Origin* LocSAT::locate(PickList& picks) throw(Core::GeneralException) {
	if (_locateEvent) delete _locateEvent;
	_locateEvent = new Internal::LocSAT;
	_locateEvent->setOrigin(0.0, 0.0, 0.0);
	_locateEvent->setOriginTime(0.0);
	setLocatorParams(LP_USE_LOCATION, "FALSE");

	return fromPicks(picks);
}


DataModel::Origin* LocSAT::fromPicks(PickList& picks){
	if ( _fixDepth ) {
		_locator_params->fixing_depth = _fixedDepth;
		strcpy(&_locator_params->fix_depth, "y");
	}
	else
		strcpy(&_locator_params->fix_depth, "n");

	_locateEvent->setLocatorParams(_locator_params);

	int i = 0;

	for (PickList::iterator it = picks.begin(); it != picks.end(); ++it){
		DataModel::Pick* pick = it->first.get();
		DataModel::SensorLocation* sloc = getSensorLocation(pick);

		if (sloc){
			std::string stationID = pick->waveformID().networkCode()+"."+
			                        pick->waveformID().stationCode()+"."+
			                        pick->waveformID().locationCode();

			_locateEvent->addSite(stationID.c_str(),
			                      sloc->latitude(), sloc->longitude(),
			                      sloc->elevation());

			std::string phase;
			try { phase = pick->phaseHint().code(); }
			catch(...) { phase = "P"; }

			_locateEvent->addArrival(i++, stationID.c_str(),
			                         phase.c_str(),
			                         (double)pick->time().value(), ARRIVAL_TIME_ERROR, 1);

			// Set backazimuth
			try {
				float az = pick->backazimuth().value();
				float delaz;
				try { delaz = pick->backazimuth().uncertainty(); }
				// Default delaz
				catch ( ... ) { delaz = 0; }
				_locateEvent->setArrivalAzimuth(az,delaz,1);
			}
			catch ( ... ) {}

			// Set slowness
			try {
				float slo = pick->horizontalSlowness().value();
				float delslo;

				try { delslo = pick->horizontalSlowness().uncertainty(); }
				// Default delaz
				catch ( ... ) { delslo = 0; }

				_locateEvent->setArrivalSlowness(slo, delslo, 1);
			}
			catch ( ... ) {}

#ifdef LOCSAT_TESTING
			SEISCOMP_DEBUG("pick station: %s", stationID.c_str());
			SEISCOMP_DEBUG("station lat: %.2f", (double)sloc->latitude());
			SEISCOMP_DEBUG("station lon: %.2f", (double)sloc->longitude());
			SEISCOMP_DEBUG("station elev: %.2f", (double)sloc->elevation());
#endif
		}
		else
			throw StationNotFoundException("station '" + pick->waveformID().networkCode() +
			                               "." + pick->waveformID().stationCode() + 
			                               "." + pick->waveformID().locationCode() + "' not found");
	}

	_locateEvent->setLocatorParams(_locator_params);
	Internal::Loc* newLoc = _locateEvent->doLocation();

	DataModel::Origin* origin = loc2Origin(newLoc);

	if ( origin ) {
		std::set<std::string> stationsUsed;
		std::set<std::string> stationsAssociated;

		for ( int i = 0; i < newLoc->arrivalCount; ++i ) {
			size_t arid = (size_t)newLoc->arrival[i].arid;
			if ( arid >= picks.size() ) continue;
			Pick* p = picks[arid].first.get();

			if ( (size_t)i < origin->arrivalCount() ) {
				origin->arrival(i)->setPickID(p->publicID());

				stationsAssociated.insert(p->waveformID().networkCode() + "." + p->waveformID().stationCode());

				try {
					if ( origin->arrival(i)->weight() == 0 ) continue;
				}
				catch ( ... ) {}

				stationsUsed.insert(p->waveformID().networkCode() + "." + p->waveformID().stationCode());
			}
		}

		try {
			origin->quality().setUsedStationCount(stationsUsed.size());
			origin->quality().setAssociatedStationCount(stationsAssociated.size());
		}
		catch ( ... ) {
			OriginQuality oq;
			oq.setUsedStationCount(stationsUsed.size());
			oq.setAssociatedStationCount(stationsAssociated.size());
			origin->setQuality(oq);
		}
	}

	if ( newLoc) free(newLoc);
	delete _locateEvent;
	_locateEvent = NULL;

	if ( origin && _useArrivalRMSAsTimeError ) {
		double sig = 0.0;
		int used = 0;
		for ( size_t i = 0; i < origin->arrivalCount(); ++i )
			if ( origin->arrival(i)->weight() >= 0.5 ) {
				++used;
				sig += origin->arrival(i)->timeResidual() * origin->arrival(i)->timeResidual();
			}

		DataModel::OriginPtr originPtr(origin);
		origin = relocate(origin, used > 4?sqrt(sig / (used - 4)):ARRIVAL_TIME_ERROR);
	}

	return origin;
}


DataModel::Origin* LocSAT::relocate(const DataModel::Origin* origin, double timeError) {

	if (!origin) return NULL;

	setLocatorParams(LP_USE_LOCATION, "TRUE");

	if (_locateEvent) delete _locateEvent;
	_locateEvent = new Internal::LocSAT;

	double depth = 0.0;
	try { depth = origin->depth().value(); } catch (...){}

	_locateEvent->setOrigin(origin->latitude().value(),
                            origin->longitude().value(),
                            depth );

	_locateEvent->setOriginTime((double)origin->time().value());

	if ( ! loadArrivals(origin, timeError)) {
		delete _locateEvent;
		_locateEvent = NULL;
		return NULL;
	}

	if ( _fixDepth ) {
		_locator_params->fixing_depth = _fixedDepth;
		strcpy(&_locator_params->fix_depth, "y");
	}
	else
		strcpy(&_locator_params->fix_depth, "n");

	_locateEvent->setLocatorParams(_locator_params);
	Internal::Loc* newLoc = _locateEvent->doLocation();

	DataModel::Origin* result = loc2Origin(newLoc);

	if ( result ) {
		std::set<std::string> stationsUsed;
		std::set<std::string> stationsAssociated;

		for ( int i = 0; i < newLoc->arrivalCount; ++i ) {
			size_t arid = (size_t)newLoc->arrival[i].arid;
			if ( arid >= origin->arrivalCount() ) continue;

			if ( (size_t)i < result->arrivalCount() ) {
				result->arrival(i)->setPickID(origin->arrival(arid)->pickID());
				DataModel::Pick* p = Pick::Find(result->arrival(i)->pickID());

				if ( p != NULL )
					stationsAssociated.insert(p->waveformID().networkCode() + "." + p->waveformID().stationCode());

				try {
					if ( result->arrival(i)->weight() == 0 ) continue;
				}
				catch ( ... ) {}

				if ( p != NULL )
					stationsUsed.insert(p->waveformID().networkCode() + "." + p->waveformID().stationCode());
			}
		}

		try {
			result->quality().setUsedStationCount(stationsUsed.size());
			result->quality().setAssociatedStationCount(stationsAssociated.size());
		}
		catch ( ... ) {
			OriginQuality oq;
			oq.setUsedStationCount(stationsUsed.size());
			oq.setAssociatedStationCount(stationsAssociated.size());
			result->setQuality(oq);
		}
	}

	if ( newLoc) free(newLoc);
	delete _locateEvent;
	_locateEvent = NULL;

	return result;
}


DataModel::Origin* LocSAT::relocate(const DataModel::Origin* origin) throw(Core::GeneralException) {
	DataModel::Origin* o = relocate(origin, ARRIVAL_TIME_ERROR);
	if ( o && _useArrivalRMSAsTimeError ) {
		double sig = 0.0;
		int used = 0;
		for ( size_t i = 0; i < o->arrivalCount(); ++i )
			if ( o->arrival(i)->weight() >= 0.5 ) {
				++used;
				sig += o->arrival(i)->timeResidual() * o->arrival(i)->timeResidual();
			}

		DataModel::OriginPtr origin(o);
		o = relocate(o, used > 4?sqrt(sig / (used - 4)):ARRIVAL_TIME_ERROR);
	}

	return o;
}


static bool atTransitionPtoPKP(const DataModel::Arrival* arrival)
{
	return (arrival->distance() > 106.9 && arrival->distance() < 111.1);
}


// Let this be a local hack for the time being. See the same routine in Autoloc
static bool travelTimeP(double lat, double lon, double depth, double delta, double azi, TravelTime &tt)
{
	static Seiscomp::TravelTimeTable ttt;

	double lat2, lon2;
	Math::Geo::delandaz2coord(delta, azi, lat, lon, &lat2, &lon2);

	Seiscomp::TravelTimeList
		*ttlist = ttt.compute(lat, lon, depth, lat2, lon2, 0);

	if ( ttlist == NULL || ttlist->empty() )
		return false;

	for (Seiscomp::TravelTimeList::iterator
	     it = ttlist->begin(); it != ttlist->end(); ++it) {
		tt = *it;
		if (delta < 114)
			// for  distances < 114, allways take 1st arrival
			break;
		if (tt.phase.substr(0,2) != "PK")
			// for  distances >= 114, skip Pdiff etc., take first
			// PKP*, PKiKP*
			continue;
		break;
	}
	delete ttlist;

	return true;
}


bool LocSAT::loadArrivals(const DataModel::Origin* origin, double timeError) {

	if ( ! origin)
		return false;

#ifdef LOCSAT_TESTING
	SEISCOMP_DEBUG("Load arrivals:");
#endif
	for (unsigned int i = 0; i < origin->arrivalCount(); i++){

		DataModel::Arrival* arrival = origin->arrival(i);
		DataModel::Pick* pick = getPick(arrival);
		if (!pick){
			throw PickNotFoundException("pick '" + arrival->pickID() + "' not found");
		}
		double traveltime = double(pick->time().value() - origin->time().value());

		int defining = 1;

		try{
			double arrivalWeight = arrival->weight();
// 			double arrivalWeight = rand()/(RAND_MAX + 1.0);
			if (arrivalWeight <= _minArrivalWeight){
				defining = 0;
			}
			// work around problem related to discontinuity in the travel-time tables
			// at the P->PKS transition
			if (atTransitionPtoPKP(arrival))
				defining = 0;
		}
		catch (...) {}

		DataModel::SensorLocation *sloc = getSensorLocation(pick);
		if (!sloc){
			throw StationNotFoundException("station '" + pick->waveformID().networkCode() +
			                               "." + pick->waveformID().stationCode() + "' not found");
		}

		std::string phaseCode;
		try {
			phaseCode = arrival->phase().code();
			// Rename P to PKP where appropriate. A first arrival
			// with a traveltime of > 1000 s observed at distances
			// > 110 deg is definitely not P but PKP, as PKP
			// traveltime is always well above 1000 s and P traveltime
			// is always well below.
			if ((phaseCode=="P" || phaseCode=="P1") && arrival->distance() > 110 && traveltime > 1000) {
				phaseCode="PKP";
			}
		}
		catch (...) {
			try {
				phaseCode = pick->phaseHint().code();
			}
			catch (...) {
				phaseCode = "P";
			}
		}

		/*
		// This is essentially a whitelist for automatic phases. Only
		// P and PKP are currently deemed acceptable automatic phases.
		// Others may be associated but cannot be used because of
		// this:
		if (pick->evaluationMode() == DataModel::AUTOMATIC && 
		    ! ( phaseCode == "P" || phaseCode == "PKP") ) // TODO make this configurable
			defining=0;
		*/

		std::string stationID = pick->waveformID().networkCode()+"."+
				                pick->waveformID().stationCode()+"."+
				                pick->waveformID().locationCode();

#ifdef LOCSAT_TESTING
		SEISCOMP_DEBUG(" [%s] set phase to %s", stationID.c_str(), phaseCode.c_str());
#endif

		_locateEvent->addSite(stationID.c_str(),
		                      sloc->latitude(), sloc->longitude(), sloc->elevation());

		_locateEvent->addArrival(i, stationID.c_str(),
		                         phaseCode.c_str(),
		                         (double)pick->time().value(), timeError, defining);

		// Set backazimuth
		try {
			float az = pick->backazimuth().value();
			float delaz;
			try { delaz = pick->backazimuth().uncertainty(); }
			// Default delaz
			catch ( ... ) { delaz = 0; }
			_locateEvent->setArrivalAzimuth(az,delaz,1);
		}
		catch ( ... ) {}

		// Set slowness
		try {
			float slo = pick->horizontalSlowness().value();
			float delslo;

			try { delslo = pick->horizontalSlowness().uncertainty(); }
			// Default delaz
			catch ( ... ) { delslo = 0; }

			_locateEvent->setArrivalSlowness(slo, delslo,1);
		}
		catch ( ... ) {}
	}

	return true;
}


DataModel::Origin* LocSAT::loc2Origin(Internal::Loc* loc){
	if ( loc == NULL ) return NULL;

	DataModel::Origin* origin = _newOriginID.empty()
	             ?DataModel::Origin::Create()
	             :DataModel::Origin::Create(_newOriginID);
	if (!origin) return NULL;

	DataModel::CreationInfo ci;
	ci.setCreationTime(Core::Time().gmt());
	origin->setCreationInfo(ci);

	origin->setMethodID("LOCSAT");
	origin->setEarthModelID(_tablePrefix);
	origin->setLatitude(DataModel::RealQuantity(loc->origin->lat, sqrt(loc->origerr->syy), Core::None, Core::None, Core::None));
	origin->setLongitude(DataModel::RealQuantity(loc->origin->lon, sqrt(loc->origerr->sxx), Core::None, Core::None, Core::None));
	origin->setDepth(DataModel::RealQuantity(loc->origin->depth, sqrt(loc->origerr->szz), Core::None, Core::None, Core::None));

	origin->setTime(DataModel::TimeQuantity(Core::Time(loc->origin->time), sqrt(loc->origerr->stt), Core::None, Core::None, Core::None));


	double rms = 0;
	int phaseAssocCount = 0;
	int usedAssocCount = 0;
	std::vector<double> dist;
	std::vector<double> azi;
	int depthPhaseCount = 0;

	for ( int i = 0; i < loc->arrivalCount; ++i ) {
		++phaseAssocCount;

		ArrivalPtr arrival = new DataModel::Arrival();
		// To have different pickID's just generate some based on
		// the index. They become set correctly later on.
		arrival->setPickID(Core::toString(i));

		if (loc->locator_errors[i].arid != 0 ||
		    ! strcmp(loc->assoc[i].timedef, "n") ){
			arrival->setWeight(0.0);
// 			SEISCOMP_DEBUG("arrival %d : setting weight to 0.0 -  because it was not used in locsat", i);
		}
		else {
			arrival->setWeight(1.0);
			rms += (loc->assoc[i].timeres * loc->assoc[i].timeres);
			dist.push_back(loc->assoc[i].delta);
			azi.push_back(loc->assoc[i].esaz);
			++usedAssocCount;
		}

		arrival->setDistance(loc->assoc[i].delta);
		arrival->setTimeResidual(loc->assoc[i].timeres < -990. ? 0. : loc->assoc[i].timeres);
		arrival->setAzimuth(loc->assoc[i].esaz);
		arrival->setPhase(Phase(loc->assoc[i].phase));
		if (arrival->phase().code()[0] == 'p' || arrival->phase().code()[0] == 's')
			if (arrival->weight() > 0.5)
				depthPhaseCount++;

		// This is a workaround for what seems to be a problem with LocSAT,
		// namely, that in a narrow distance range around 108 degrees
		// sometimes picks suddenly have a residual of > 800 sec after
		// relocation. The reason is not clear.
		if ( arrival->timeResidual() > 800 && \
		   ( arrival->phase().code()=="P" || arrival->phase().code()=="Pdiff" ) && \
		     atTransitionPtoPKP(arrival.get())) {

			TravelTime tt;
			if ( travelTimeP(origin->latitude(), origin->longitude(), origin->depth(), arrival->distance(), arrival->azimuth(), tt) ) {
				double res = loc->arrival[i].time - double(origin->time().value() + Core::TimeSpan(tt.time));
				arrival->setTimeResidual(res);
			}
		}


		if ( !origin->add(arrival.get()) )
			SEISCOMP_DEBUG("arrival not added for some reason");
	}

	DataModel::OriginQuality originQuality;

	originQuality.setAssociatedPhaseCount(phaseAssocCount);
	originQuality.setUsedPhaseCount(usedAssocCount);
	originQuality.setDepthPhaseCount(depthPhaseCount);

	if (phaseAssocCount > 0) {
		rms /= usedAssocCount;
		originQuality.setStandardError(sqrt(rms));
	}

	std::sort(azi.begin(), azi.end());
	azi.push_back(azi.front()+360.);
	double azGap = 0.;
	if (azi.size() > 2)
		for (size_t i = 0; i < azi.size()-1; i++)
			azGap = (azi[i+1]-azi[i]) > azGap ? (azi[i+1]-azi[i]) : azGap;
	if (0. < azGap && azGap < 360.)
		originQuality.setAzimuthalGap(azGap);

	std::sort(dist.begin(), dist.end());
	originQuality.setMinimumDistance(dist.front());
	originQuality.setMaximumDistance(dist.back());
	originQuality.setMedianDistance(dist[dist.size()/2]);

#ifdef LOCSAT_TESTING
	SEISCOMP_DEBUG("--- Confidence region at %4.2f level: ----------------", 0.9);
	SEISCOMP_DEBUG("Semi-major axis:   %8.2f km", loc->origerr->smajax);
	SEISCOMP_DEBUG("Semi-minor axis:   %8.2f km", loc->origerr->sminax);
	SEISCOMP_DEBUG("Major axis strike: %8.2f deg. clockwise from North", loc->origerr->strike);
	SEISCOMP_DEBUG("Depth error:       %8.2f km", loc->origerr->sdepth);
	SEISCOMP_DEBUG("Orig. time error:  %8.2f sec", loc->origerr->stime);
	SEISCOMP_DEBUG("--- OriginQuality ------------------------------------");
	SEISCOMP_DEBUG("DefiningPhaseCount: %d", originQuality.definingPhaseCount());
	SEISCOMP_DEBUG("PhaseAssociationCount: %d", originQuality.phaseAssociationCount());
// 	SEISCOMP_DEBUG("ArrivalCount: %d", originQuality.stationCount());
	SEISCOMP_DEBUG("Res. RMS: %f sec", originQuality.rms());
	SEISCOMP_DEBUG("AzimuthalGap: %f deg", originQuality.azimuthalGap());
	SEISCOMP_DEBUG("originQuality.setMinimumDistance: %f deg", originQuality.minimumDistance());
	SEISCOMP_DEBUG("originQuality.setMaximumDistance: %f deg", originQuality.maximumDistance());
	SEISCOMP_DEBUG("originQuality.setMedianDistance:  %f deg", originQuality.medianDistance());
	SEISCOMP_DEBUG("------------------------------------------------------");
#endif

	//! --------------------------------------------------

	origin->setQuality(originQuality);

	_errorEllipsoid.smajax = loc->origerr->smajax;
	_errorEllipsoid.sminax = loc->origerr->sminax;
	_errorEllipsoid.strike = loc->origerr->strike;
	_errorEllipsoid.sdepth = loc->origerr->sdepth;
	_errorEllipsoid.stime  = loc->origerr->stime;
	_errorEllipsoid.sdobs  = loc->origerr->sdobs;
	_errorEllipsoid.conf   = loc->origerr->conf;

	return origin;
}


void LocSAT::setDefaultLocatorParams(){
	_locator_params->cor_level = 0;

	_locator_params->use_location 	= TRUE;
	_locator_params->fix_depth	= 'n';
	_locator_params->fixing_depth	= 20.0;
	_locator_params->verbose	= 'n';
	_locator_params->lat_init 	= 999.9;
	_locator_params->lon_init 	= 999.9;
	_locator_params->depth_init 	= 20.0;	
	_locator_params->conf_level 	= 0.90;
	_locator_params->damp		= -1.00;
	_locator_params->est_std_error 	= 1.00;
	_locator_params->num_dof	= 9999;
	_locator_params->max_iterations = 100;

	_minArrivalWeight               = 0.5;
	_useArrivalRMSAsTimeError       = false;

	strcpy(_locator_params->outfile_name, "ls.out");
}


void LocSAT::setDefaultProfile(const std::string &prefix) {
	_defaultTablePrefix = prefix;
}


std::string LocSAT::currentDefaultProfile() {
	return _defaultTablePrefix;
}


LocatorInterface::IDList LocSAT::profiles() const {
	return _profiles;
}

void LocSAT::setProfile(const std::string &prefix) {
	if ( prefix.empty() ) return;
	_tablePrefix = prefix;
	strcpy(_locator_params->prefix, (Environment::Instance()->shareDir() + "/locsat/tables/" + _tablePrefix).c_str());
}


const char* LocSAT::getLocatorParams(int param){

	char* value = new char[1024];

	switch(param){
	
	case LP_USE_LOCATION:
		if (_locator_params->use_location == TRUE)
			strcpy(value, "TRUE");
		else
			strcpy(value, "FALSE");
	break;

	case LP_FIX_DEPTH:
		strcpy(value, &_locator_params->fix_depth);
	break;
	
	case LP_FIXING_DEPTH:
		sprintf(value, "%7.2f", _locator_params->fixing_depth);
	break;
	
	case LP_VERBOSE:
		strcpy(value, &_locator_params->verbose);
	break;

	case LP_PREFIX:
		strcpy(value, _locator_params->prefix);
	break;
	
	case LP_MAX_ITERATIONS:
		sprintf(value, "%d", _locator_params->max_iterations);
	break;

	case LP_EST_STD_ERROR:
		sprintf(value, "%7.2f", _locator_params->est_std_error);
	break;

	case LP_NUM_DEG_FREEDOM:
		sprintf(value, "%d", _locator_params->num_dof);
	break;

	case LP_MIN_ARRIVAL_WEIGHT:
		sprintf(value, "%7.2f", _minArrivalWeight);
	break;

	default:
		SEISCOMP_ERROR("getLocatorParam: wrong Parameter: %d", param);
		return "error";
	}
return value;
}


void LocSAT::setLocatorParams(int param, const char* value){

	switch(param){
	
	case LP_USE_LOCATION:
		if (!strcmp(value, "TRUE"))
			_locator_params->use_location = TRUE;
		else
			_locator_params->use_location = FALSE;
	break;

	case LP_FIX_DEPTH:
		strcpy(&_locator_params->fix_depth, value);
	break;
	
	case LP_FIXING_DEPTH:
		_locator_params->fixing_depth = atof(value);
	break;
	
	case LP_VERBOSE:
		strcpy(&_locator_params->verbose, value);
	break;
	
	case LP_PREFIX:
		strcpy(_locator_params->prefix, value);
	break;
	
	case LP_MAX_ITERATIONS:
		_locator_params->max_iterations = atoi(value);
	break;

	case LP_EST_STD_ERROR:
		_locator_params->est_std_error = atof(value);
	break;

	case LP_NUM_DEG_FREEDOM:
		_locator_params->num_dof = atoi(value);
	break;

	case LP_MIN_ARRIVAL_WEIGHT:
		_minArrivalWeight = atof(value);
	break;

	case LP_RMS_AS_TIME_ERROR:
		_useArrivalRMSAsTimeError = !strcmp(value, "TRUE");
	break;

	default:
		SEISCOMP_ERROR("setLocatorParam: wrong Parameter: %d", param);
	}
}


}// of namespace Seiscomp
