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




# define SEISCOMP_COMPONENT Autoloc
#include <seiscomp3/logging/log.h>
#include <seiscomp3/core/datetime.h>
#include <seiscomp3/math/geo.h>
#include <seiscomp3/math/mean.h>

#include <assert.h>
#include <math.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <map>

#include "util.h"
#include "nucleator.h"
#include "datamodel.h"
#include "sc3adapters.h"


namespace Autoloc {

double distance(const Station* s1, const Station* s2)
{
	double delta, az, baz;
	delazi(s1->lat, s1->lon, s2->lat, s2->lon, delta, az, baz);
	return delta;
}

std::string printDetailed(const Origin *origin)
{
	return printOrigin(origin, false);
}

std::string printOneliner(const Origin *origin)
{
	return printOrigin(origin, true);
}

bool automatic(const Pick *pick)
{
	return pick->status == Pick::Automatic;
}

bool ignored(const Pick *pick)
{
	return pick->status == Pick::IgnoredAutomatic;
}

bool manual(const Pick *pick)
{
	return pick->status == Pick::Manual;
}

char statusFlag(const Pick *pick)
{
	return automatic(pick) ? 'A' : 'M';
}

bool hasAmplitude(const Pick *pick)
{
	if (pick->amp <= 0)
		return false;
	return true;
}


bool travelTimeP(double lat1, double lon1, double dep1, double lat2, double lon2, double alt2, double delta, TravelTime &tt)
{
	static Seiscomp::TravelTimeTable ttt;

	Seiscomp::TravelTimeList
		*ttlist = ttt.compute(lat1, lon1, dep1, lat2, lon2, alt2);

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

// 1st arrival P incl. Pdiff up to 130 deg, no PKP
bool travelTimeP1(double lat1, double lon1, double dep1, double lat2, double lon2, double alt2, double delta, TravelTime &tt)
{
	if (delta > 130) // negotiable ;)
		return false;

	static Seiscomp::TravelTimeTable ttt;

	Seiscomp::TravelTimeList
		*ttlist = ttt.compute(lat1, lon1, dep1, lat2, lon2, alt2, alt2);
	tt = *(ttlist->begin());
	delete ttlist;

	return true;
}

bool travelTimePK(double lat1, double lon1, double dep1, double lat2, double lon2, double alt2, double delta, TravelTime &tt)
{
	if (delta < 30) // negotiable ;)
		return false;

	static Seiscomp::TravelTimeTable ttt;

	Seiscomp::TravelTimeList
		*ttlist = ttt.compute(lat1, lon1, dep1, lat2, lon2, alt2);

	for (Seiscomp::TravelTimeList::iterator
	     it = ttlist->begin(); it != ttlist->end(); ++it) {
		tt = *it;
		if (tt.phase.substr(0,3) == "PKP" || tt.phase == "PKiKP")
			break;
	}
	delete ttlist;

	return true;
}

TravelTime travelTimePP(double lat1, double lon1, double dep1, double lat2, double lon2, double alt2, double delta)
{
	static Seiscomp::TravelTimeTable ttt;

	Seiscomp::TravelTimeList
		*ttlist = ttt.compute(lat1, lon1, dep1, lat2, lon2, alt2);
	TravelTime tt;

	for (Seiscomp::TravelTimeList::iterator
	     it = ttlist->begin(); it != ttlist->end(); ++it) {
		tt = *it;
		if (tt.phase == "PP")
			break;
		if (tt.phase == "PnPn")
			break;
	}
	delete ttlist;

	// FIXME check if tt.phase == "PnPn" || tt.phase == "PP"
	// Otherwise throw exception
	return tt;
}

static Time str2time(const std::string &s)
{
	Seiscomp::Core::Time t;
	if ( ! t.fromString(s.c_str(), "%F %T.%f")) {
		SEISCOMP_ERROR_S("Failed to convert time string " + s);
		exit(3);
	}
	return Time(t);
}

std::string time2str(const Time &t)
{
	return sc3time(t).toString("%F %T.%f000000").substr(0,21);
}


double meandev(const Origin* origin)
{
	int arrivalCount = origin->arrivals.size();
	double cumresid = 0, cumweight = 0.;

	for(int i=0; i<arrivalCount; i++) {
		const Arrival &arr = origin->arrivals[i];
		if (arr.excluded)
			continue;
		cumresid  += fabs(arr.residual);
		cumweight += 1;
	}

	if (cumweight == 0.)
		return 0;

	return cumresid/cumweight;
}

double avgfn(double x)
{
	if (x<-1 || x>1)
		return 0;

	x *= M_PI*0.5;
	x = cos(x);
	return x*x;
}


std::string printOrigin(const Origin *origin, bool oneliner)
{
	assert(origin!=NULL);

	std::ostringstream out;

	int precision = out.precision();

	out.precision(10);

	char depthFlag = ' ';
	switch (origin->depthType) {
	case Origin::DepthMinimum:       depthFlag = 'i'; break;
	case Origin::DepthPhases:        depthFlag = 'p'; break;
	case Origin::DepthDefault:       depthFlag = 'd'; break;
	case Origin::DepthManuallyFixed: depthFlag = 'f'; break;
	default:                         depthFlag = ' ';
	}

	if (oneliner) {
		double score = originScore(origin);
		char s[200];
		std::string tstr = time2str(origin->time).substr(11);
		sprintf(s, "%-6lu %s %6.2f %7.2f %3.0f%c %4.1f %3d %3lu s=%5.1f",
			origin->id, tstr.c_str(),
			origin->lat, origin->lon, origin->dep, depthFlag,
			origin->rms(), origin->definingPhaseCount(),
			(unsigned long)origin->arrivals.size(),score);
		out << s;
	}
	else {
		out << "Detailed info for Origin " << origin->id << std::endl
		    << time2str(origin->time) << "  ";
		out.precision(5);
		out << origin->lat << "  " << origin->lon << "  ";
		out.precision(3);
		out << origin->dep << depthFlag << std::endl;

		out.setf(std::ios::right);

		int arrivalCount = origin->arrivals.size();
		for(int i=0; i<arrivalCount; i++) {
			const Arrival &arr = origin->arrivals[i];
			const Pick* pick = arr.pick.get();

			string excludedFlag;
			switch(arr.excluded) {
			case Arrival::NotExcluded:          excludedFlag = "  "; break;
			case Arrival::LargeResidual:        excludedFlag = "Xr"; break;
			case Arrival::StationDistance:      excludedFlag = "Xd"; break;
			case Arrival::ManuallyExcluded:     excludedFlag = "Xm"; break;
			case Arrival::UnusedPhase:          excludedFlag = "Xp"; break;
			case Arrival::DeterioratesSolution: excludedFlag = "X!"; break;
			case Arrival::TemporarilyExcluded:  excludedFlag = "Xt"; break;
			default:                            excludedFlag = "X ";
			}
			if ( ! pick->station()) {
				out << pick->id << "   missing station information" << std::endl;
				continue;
			}

			std::string net = pick->station()->net;
			std::string sta = pick->station()->code;

			out.precision(1);
			out     << std::left << std::setw(4) << (i+1)
				<< std::setw(6) << sta
				<< std::setw(3) << net;
			out.precision(2);
			out	<< std::right << std::fixed << std::setw(6) << arr.distance;
			out.precision(0);
			out	<< std::setw(4) << arr.azimuth
				<< std::left << " "
				<< std::setw(11) << time2str(pick->time).substr(11);
			out.precision(1);
			out	<< std::right
				<< std::setiosflags(std::ios::fixed)
				<< std::setw(6) << arr.residual << " "
				<< ((arr.pick->status == Pick::Automatic) ?"A":"M") // << " "
				<< excludedFlag << " "
				<< std::left << std::setw(6) << arr.phase << " "
				<< (arr.pick->xxl ? "XXL":"   ");
			out.precision(1);
			out	<< "  " << std::right << std::fixed << std::setw(5) << arr.pick->snr;;
			out.precision(2);
			out	<< " " << arr.score << " -";
			out	<< " " << arr.tscore;
			out	<< " " << arr.ascore;
			out	<< " " << arr.dscore;
			out	<< std::endl;
		}

		out.precision(1);
		out << std::endl << "RMS   = " << origin->rms() << std::endl;
		out << "MD    = " << meandev(origin) << std::endl;
		out << "PGAP  = " << origin->quality.aziGapPrimary << std::endl;
		out << "SGAP  = " << origin->quality.aziGapSecondary << std::endl;
		out << "SCORE = " << originScore(origin) << std::endl;
		out << "errorEllipsoid";
		out << "  smaj=" << origin->errorEllipsoid.semiMajorAxis;
		out << "  smin=" << origin->errorEllipsoid.semiMinorAxis;
		out << "  strike=" << origin->errorEllipsoid.strike;
		out << "  sdep=" << origin->errorEllipsoid.sdepth;
		out << "  stime="  << origin->errorEllipsoid.stime         << std::endl;
		out << "preliminary = "  << (origin->preliminary ? "true":"false") << std::endl;

		out.precision(precision);
	}
	return out.str();
}


namespace Utils {

StationDB *readStationLocations(const std::string &fname)
{
	StationDB *stations = new StationDB;
	std::string code, net;
	double lat, lon, alt;

	std::ifstream ifile(fname.c_str());
	while (ifile >> net >> code >> lat >> lon >> alt) {

		Station* station = new Station(code, net, lat, lon, alt);
		station->maxNucDist = 180; // TODO make this default configurable
		station->used = true;
		std::string key = net + "." + code;
		stations->insert(std::pair<std::string, Station*>(key, station));
	}

	if ( stations->size() == 0) {
		SEISCOMP_ERROR_S("No stations read from file " + fname);
		delete stations;
		return NULL;
	}

	return stations;
}


/*
struct _StationConfig {
	bool used;
	float maxnucdist, maxlocdist;
};

bool readStationConfig(StationDB *stations, const std::string &fname)
{
	if (fname.size() == 0)
		return false;

	std::string code, net;
	int usage;
	float maxnucdist;
	float maxlocdist=180; // TODO: make this configurable (not urgent)
	std::ifstream ifile(fname.c_str());
	if ( ! ifile.good()) {
		SEISCOMP_ERROR("Failed to open station config file '%s'",
			       fname.c_str());
		return false;
	}

	std::map<std::string, _StationConfig> config;

	// TODO: This file format should support comments
        while (ifile >> net >> code >> usage >> maxnucdist) {

		_StationConfig c = { usage>0, maxnucdist, maxlocdist };
		config[net + " " + code] = c;
        }

	for (StationDB::iterator
	     it = stations->begin(); it !=stations->end(); ++it) {

		Station *station = (*it).second;

		std::vector<std::string> patterns;
		patterns.push_back(station->net + " " + station->code);
		patterns.push_back(station->net + " *");
		patterns.push_back("* " + station->code);
		patterns.push_back("* *");

		for (std::vector<std::string>::iterator
		     it = patterns.begin(); it != patterns.end(); ++it) {

			const std::string &pattern = *it;
			if (config.find(pattern) == config.end())
				continue;

			station->maxNucDist = config[pattern].maxnucdist;
			station->maxLocDist = config[pattern].maxlocdist;
			station->used = config[pattern].used;
			SEISCOMP_DEBUG("Station %-8s pattern %-8s config: usage=%d maxnucdist=%g",
					(station->net + " " + station->code).c_str(),
					pattern.c_str(), station->used, station->maxNucDist);
			break;
		}
	}

        return true;
}
*/

Pick* readPickLine()
{
	PickDB picks;
	std::string sta, net, cha, loc, date, time, id;
	double snr, amp, per;
	char stat;

	if ( ! (std::cin >>date >>time >>net >>sta >>cha >>loc >>snr >>amp >>per >>stat >>id) )
		return NULL;

	std::string key = net + "." + sta;
	Time ptime = str2time(date+" "+time);
	Pick *pick = new Pick(id, net, sta, ptime);
	pick->amp  = amp;
	pick->per  = per;
	pick->snr  = snr;

	switch(stat) {
	case 'A': pick->status = Pick::Automatic ; break;
	case 'C': pick->status = Pick::Confirmed ; break;
	case 'M': pick->status = Pick::Manual    ; break;
	default: SEISCOMP_ERROR("Pick status: %c", stat);
	}

	return pick;
}


PickDB readPickFile()
{
	PickDB picks;
	PickPtr pick = 0;

	while ( (pick = readPickLine()) != NULL)
		picks.push_back(pick);	

	return picks;
}

Pick::Status status(const Seiscomp::DataModel::Pick *pick) {
	try {
		switch ( pick->evaluationMode() ) {
			case Seiscomp::DataModel::AUTOMATIC:
				return Pick::Automatic;
			case Seiscomp::DataModel::MANUAL:
				return Pick::Manual;
			//case Seiscomp::DataModel::CONFIRMED_PICK:
			//	return Pick::Confirmed;
			default:
				break;
		}
	}
	catch ( ... ) {}

	return Pick::Automatic;
}

} // namespace Autoloc::Utils
} // namespace Autoloc


namespace Seiscomp {
namespace Math {
namespace Statistics {

double rms(const std::vector<double> &v, double offset /* = 0 */)
{
	unsigned int n = v.size();
	const double *f = &v[0];
	double fi, r=0;

	if(offset) {
		for(unsigned int i=0; i<n; i++, f++) {
			fi = ((*f)-offset);
			r += fi*fi;
		}
	}
	else {
		for(unsigned int i=0; i<n; i++, f++) {
			r += (*f)*(*f);
		}
	}

	return sqrt(r/n);
}

} // namespace Statistics
} // namespace Math
} // namespace Seiscomp

