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





#define SEISCOMP_COMPONENT Autopick

#include <seiscomp3/logging/log.h>
#include <seiscomp3/core/strings.h>
#include <seiscomp3/datamodel/configstation.h>
#include <seiscomp3/datamodel/parameterset.h>
#include <seiscomp3/datamodel/parameter.h>
#include <seiscomp3/processing/processor.h>

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <ctype.h>

#include "stationconfig.h"


#ifdef _MSC_VER
#define isWhiteSpace isspace
#else
#define isWhiteSpace std::isspace
#endif


namespace {


void parseToks(std::vector<std::string> &toks, const std::string &line) {
	size_t lit = 0;
	size_t lastit = lit;
	std::string tok;
	bool skipWhitespaces = false;

	while ( lit < line.size() ) {
		if ( isWhiteSpace(line[lit]) && !skipWhitespaces ) {
			if ( !tok.empty() ) {
				toks.push_back(tok);
				tok.clear();
			}

			++lit;
			while ( lit < line.size() && isWhiteSpace(line[lit]) )
				++lit;
			lastit = lit;
		}
		else if ( line[lit] == '\"' ) {
			if ( skipWhitespaces ) {
				skipWhitespaces = false;
				if ( !tok.empty() ) {
					toks.push_back(tok);
					tok.clear();
				}

				++lit;
				while ( lit < line.size() && isWhiteSpace(line[lit]) )
					++lit;
				lastit = lit;
			}
			else {
				skipWhitespaces = true;
				++lit;
				lastit = lit;
			}
		}
		else {
			tok += line[lit];
			++lit;
		}
	}

	if ( lastit != lit ) toks.push_back(line.substr(lastit, lit-lastit));
}


}


namespace Seiscomp {
namespace Applications {
namespace Picker {


StreamConfig::StreamConfig() {}


StreamConfig::StreamConfig(double on, double off, double tcorr, const std::string &f)
: triggerOn(on), triggerOff(off), timeCorrection(tcorr), filter(f) {}


StationConfig::StationConfig() {}


void StationConfig::setDefault(const StreamConfig &entry) {
	_default = entry;
}


void StationConfig::read(const Seiscomp::Config *localConfig, const DataModel::ConfigModule *module) {
	if ( module == NULL ) return;

	for ( size_t j = 0; j < module->configStationCount(); ++j ) {
		DataModel::ConfigStation* station = module->configStation(j);
		for ( size_t k = 0; k < station->setupCount(); ++k ) {
			DataModel::Setup* setup = station->setup(k);
			if ( !setup->enabled() )
				continue;
	
			DataModel::ParameterSet* ps = NULL;
			try {
				ps = DataModel::ParameterSet::Find(setup->parameterSetID());
			}
			catch ( Core::ValueException ) {
				continue;
			}

			if ( !ps ) {
				SEISCOMP_ERROR("Cannot find parameter set %s", setup->parameterSetID().c_str());
				continue;
			}
	
			std::string net, sta, loc, cha, filter = _default.filter;
			double trigOn = *_default.triggerOn, trigOff = *_default.triggerOff;
			double tcorr = *_default.timeCorrection;

			net = station->networkCode();
			sta = station->stationCode();

			Processing::ParametersPtr parameters = new Processing::Parameters;
			parameters->readFrom(ps);

			Processing::Settings settings(module->name(), net, sta, "", "", localConfig, parameters.get());

			settings.getValue(loc, "detecLocid");
			settings.getValue(cha, "detecStream");
			settings.getValue(trigOn, "trigOn");
			settings.getValue(trigOff, "trigOff");
			settings.getValue(tcorr, "timeCorr");
			if ( settings.getValue(filter, "detecFilter") ) {
				if ( filter.empty() )
					filter = _default.filter;
			}
	
			if ( !cha.empty() ) {
				StreamConfig &sc = _stationConfigs[Key(net,sta)];
				sc.updatable = true;
				sc.locCode = loc;
				sc.channel = cha;
				sc.triggerOn = trigOn;
				sc.triggerOff = trigOff;
				sc.timeCorrection = tcorr;
				sc.filter = filter;
				sc.parameters = parameters;
			}
		}
	}
}


bool StationConfig::read(const std::string &staConfFile) {
	#define NET    0
	#define STA    1
	#define TON    2
	#define TOFF   3
	#define TCORR  4
	#define FILTER 5

	std::ifstream ifile(staConfFile.c_str());
	if ( !ifile.good()) {
		SEISCOMP_ERROR("Failed to open station config file %s", staConfFile.c_str());
		return false;
	}

	size_t iline = 0;
	while ( !ifile.eof() ) {
		std::string line;
		std::getline(ifile, line);
		Core::trim(line);

		++iline;

		// Skip empty lines
		if ( line.empty() ) continue;

		// Skip comments
		if ( line[0] == '#' ) continue;

		std::vector<std::string> toks;
		parseToks(toks, line);

		if ( toks.size() != 6 ) {
			SEISCOMP_ERROR("%s:%lu: error: invalid token size, expected 6 tokens", staConfFile.c_str(), (unsigned long)iline);
			return false;
		}

		Key key = Key(toks[NET],toks[STA]);

		StreamConfig *sc;
		bool newEntry;

		ConfigMap::iterator it = _stationConfigs.find(key);
		if ( it != _stationConfigs.end() ) {
			sc = &it->second;
			newEntry = false;
		}
		else {
			sc = &_stationConfigs[key];
			newEntry = true;
		}

		double value;

		if ( toks[TON] != "." ) {
			if ( !Core::fromString(value, toks[TON]) ) {
				SEISCOMP_ERROR("%s:%lu: error: invalid trigOn '%s'", staConfFile.c_str(), (unsigned long)iline, toks[TON].c_str());
				return false;
			}
			else
				sc->triggerOn = value;
		}

		if ( toks[TOFF] != "." ) {
			if ( !Core::fromString(value, toks[TOFF]) ) {
				SEISCOMP_ERROR("%s:%lu: error: invalid trigOff '%s'", staConfFile.c_str(), (unsigned long)iline, toks[TOFF].c_str());
				return false;
			}
			else
				sc->triggerOff = value;
		}

		if ( toks[TCORR] != "." ) {
			if ( !Core::fromString(value, toks[TCORR]) ) {
				SEISCOMP_ERROR("%s:%lu: error: invalid tcorr '%s'", staConfFile.c_str(), (unsigned long)iline, toks[TCORR].c_str());
				return false;
			}
			else
				sc->timeCorrection = value;
		}

		if ( toks[FILTER] != "." )
			sc->filter = toks[FILTER];
		else
			sc->filter = "";

		sc->updatable = false;

		if ( newEntry )
			_stationConfigs[key] = *sc;
	}

	for ( ConfigMap::iterator it = _stationConfigs.begin(); it != _stationConfigs.end(); ++it ) {
		// No wildcard key?
		if ( it->first.first != "*" && it->first.second != "*" && it->second.updatable ) {
			const StreamConfig *sc = getBestWildcard(it->first.first, it->first.second);
			if ( sc != NULL ) {
				if ( sc->triggerOn ) it->second.triggerOn = *sc->triggerOn;
				if ( sc->triggerOff ) it->second.triggerOff = *sc->triggerOff;
				if ( sc->timeCorrection ) it->second.timeCorrection = *sc->timeCorrection;
				if ( !sc->filter.empty() ) it->second.filter = sc->filter;
			}
		}
	}

	/*
	for ( ConfigMap::iterator it = _stationConfigs.begin(); it != _stationConfigs.end(); ++it ) {
		if ( !it->second.updatable ) continue;
		std::cout << it->first.first << "." << it->first.second << "." << it->second.locCode
		          << "." << it->second.channel << ": " << *it->second.triggerOn << ", "
		          << *it->second.triggerOff << ", " << *it->second.timeCorrection << ", "
		          << it->second.filter << std::endl;
	}
	*/

	return true;
}


const StreamConfig *StationConfig::getBestWildcard(const std::string &net, const std::string &sta) const {
	ConfigMap::const_iterator it;

	it = _stationConfigs.find(Key(net,"*"));
	if ( it != _stationConfigs.end() )
		return &it->second;

	it = _stationConfigs.find(Key("*",sta));
	if ( it != _stationConfigs.end() )
		return &it->second;

	it = _stationConfigs.find(Key("*","*"));
	if ( it != _stationConfigs.end() )
		return &it->second;

	return NULL;
}


const StreamConfig *StationConfig::get(const std::string &net, const std::string &sta) const {
	ConfigMap::const_iterator it;
	it = _stationConfigs.find(Key(net,sta));
	if ( it != _stationConfigs.end() )
		return &it->second;

	return getBestWildcard(net, sta);
}


StationConfig::const_iterator StationConfig::begin() const {
	return _stationConfigs.begin();
}


StationConfig::const_iterator StationConfig::end() const {
	return _stationConfigs.end();
}


void StationConfig::dump() const {
	printf("Stream configuration:\n");
	ConfigMap::const_iterator it = _stationConfigs.begin();
	for ( ; it != _stationConfigs.end(); ++it ) {
		if ( it->first.first == "*" || it->first.second == "*" ) continue;
		std::string streamID;
		if ( it->second.channel.empty() )
			streamID = it->first.first + "." + it->first.second;
		else
			streamID = it->first.first + "." + it->first.second + "." + it->second.locCode + "." + it->second.channel;
		printf("%-16s", streamID.c_str());
		if ( it->second.triggerOn )
			printf("%.2f ", *it->second.triggerOn);
		else
			printf("? ");

		if ( it->second.triggerOff )
			printf("%.2f ", *it->second.triggerOff);
		else
			printf("? ");

		if ( it->second.timeCorrection )
			printf("%.2f ", *it->second.timeCorrection);
		else
			printf("? ");

		if ( !it->second.filter.empty() )
			printf("%s", it->second.filter.c_str());
		else
			printf("?");

		printf("\n");
	}
}


}
}
}
