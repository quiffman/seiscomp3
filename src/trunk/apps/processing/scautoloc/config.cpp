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




#define SEISCOMP_COMPONENT Autoloc
#include <seiscomp3/logging/log.h>

#include "autoloc.h"

using namespace std;

namespace Autoloc {


StationConfig::StationConfig()
{
	string defaultkey = "* *";
	_entry[defaultkey] = Entry();
}

bool StationConfig::read(const std::string &fname)
{
	char line[100];
	string defaultkey = "* *";
	_entry[defaultkey] = Entry();

	ifstream ifile(fname.c_str());
	if ( ! ifile.good()) {
		SEISCOMP_ERROR_S("Failed to open station config file "+fname);
		return false;
	}

	Entry entry;
	while( ! ifile.getline(line, 100).eof() ) {
		char net[10],sta[10];
		int n = sscanf(line, "%8s %8s %d %f", net, sta, &entry.usage, &entry.maxNucDist);
		if (n!=4) break;
		string key = string(net)+string(" ")+string(sta);
		_entry[key] = entry;
	}

	return true;
}


const StationConfig::Entry&
StationConfig::get(const string &net, const string &sta) const
{
	string key = net+" "+sta;

	vector<string> patterns;
	patterns.push_back(net + " " + sta);
	patterns.push_back(net + " *");
	patterns.push_back("* " + sta);
	patterns.push_back("* *");

	for (vector<string>::iterator
	     it = patterns.begin(); it != patterns.end(); ++it) {

		const string &pattern = *it;
		map<string, Entry>::const_iterator mit = _entry.find(pattern);
		if (mit == _entry.end())
			continue;

		const Entry &e = (*mit).second;
		SEISCOMP_DEBUG("Station %-8s pattern %-8s config: usage=%d maxnucdist=%g",
			       (net + " " + sta).c_str(), pattern.c_str(), e.usage, e.maxNucDist);

		return e;
	}

// This should never be executed:
//	string defaultkey = "* *";
//	return _entry[defaultkey];
}

Autoloc3::Config::Config()
{
	maxAge = 6*3600;
	goodRMS = 1.5;
	maxRMS  = 3.5;
	dynamicPickThresholdInterval = 3600;
	maxResidualUse = 7.0;
	maxResidualKeep = 3*maxResidualUse;
	maxAziGapSecondary = 360; // 360 means no SGAP restriction
	thresholdXXL = 10000.;
	deadTimeXXL = 120.;
	maxStaDist = 180;
	maxStaDistXXL = 15;
	defaultMaxNucDist = 180;
	minPhaseCount = 6;
	minPhaseCountXXL = 4;
	minScore = 8;
	defaultDepth = 10;
	tryDefaultDepth = true;
	adoptManualDepth = false;
	minimumDepth = 5;
	maxDepth = 700.0;
	minPickSNR = 3;
	minPickAffinity = 0.05;
	minStaCountIgnorePKP = 15;
	minScoreBypassNucleator = 40;
	maxAllowedFakeProbability = 0.2; // TODO make this configurable
	distSlope = 1.5; // TODO: Make this configurable after testing
	offline = true;
	test = true;
	playback = false;
	cleanupInterval = 3600;
	aggressivePKP = true;
	useManualOrigins = false;
	useImportedOrigins = false;
	// If true, more exotic phases like PcP, SKKP etc. will be reported.
	// By default, only P/PKP will be reported. Internally, of course, the
	// other phases are also associated to avoid fakes. They also show up
	// in the log files
	// FIXME: Note that this is a temporary HACK
	reportAllPhases = false;

	maxRadiusFactor = 1;
	networkType = Autoloc::GlobalNetwork;

	publicationIntervalTimeSlope = 0.5;
	publicationIntervalTimeIntercept = 0;
	publicationIntervalPickCount = 20;
}

void Autoloc3::Config::dump() const
{
	SEISCOMP_INFO("Configuration:");
	SEISCOMP_INFO("defaultDepth                     %g",     defaultDepth);
	SEISCOMP_INFO("tryDefaultDepth                  %s",     tryDefaultDepth ? "true":"false");
	SEISCOMP_INFO("adoptManualDepth                 %s",     adoptManualDepth ? "true":"false");
	SEISCOMP_INFO("minimumDepth                     %g",     minimumDepth);
	SEISCOMP_INFO("minPhaseCount                    %d",     minPhaseCount);
	SEISCOMP_INFO("minPhaseCountXXL                 %d",     minPhaseCountXXL);
	SEISCOMP_INFO("minScore                         %.1f",   minScore);
	SEISCOMP_INFO("minPickSNR                       %.1f",   minPickSNR);
	SEISCOMP_INFO("maxResidual                      %.1f s", maxResidualUse);
	SEISCOMP_INFO("goodRMS                          %.1f s", goodRMS);
	SEISCOMP_INFO("maxRMS                           %.1f s", maxRMS);
	SEISCOMP_INFO("maxStaDistXXL                    %.1f °", maxStaDistXXL);
	SEISCOMP_INFO("minStaCountIgnorePKP             %d",     minStaCountIgnorePKP);
	SEISCOMP_INFO("maxAge                           %.0f s", maxAge);
	SEISCOMP_INFO("publicationIntervalTimeSlope     %.2f",   publicationIntervalTimeSlope);
	SEISCOMP_INFO("publicationIntervalTimeIntercept %.1f",   publicationIntervalTimeIntercept);
	SEISCOMP_INFO("publicationIntervalPickCount     %d",     publicationIntervalPickCount);
	SEISCOMP_INFO("reportAllPhases                  %s",     reportAllPhases ? "true":"false");
	SEISCOMP_INFO("pickLogFile                      %s",     pickLogFile.size() ? pickLogFile.c_str() : "(none)");
	SEISCOMP_INFO("dynamicPickThresholdInterval     %g",     dynamicPickThresholdInterval);
	SEISCOMP_INFO("playback                         %s",     playback ? "true":"false");
	SEISCOMP_INFO("useManualOrigins                 %s",     useManualOrigins ? "true":"false");
	SEISCOMP_INFO("useImportedOrigins               %s",     useImportedOrigins ? "true":"false");
	SEISCOMP_INFO("locatorProfile                   %s",     locatorProfile.c_str());
//	SEISCOMP_INFO("maxRadiusFactor                  %g", 	 maxRadiusFactor);
}

}  // namespace Autoloc
