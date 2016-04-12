/************************************************************************
 *                                                                      *
 * Copyright (C) 2012 OVSM/IPGP                                         *
 *                                                                      *
 * This program is free software: you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * This program is part of 'Projet TSUAREG - INTERREG IV Caraïbes'.     *
 * It has been co-financed by the European Union and le Minitère de     *
 * l'Ecologie, du Développement Durable, des Transports et du Logement. *
 *                                                                      *
 ************************************************************************/



#define SEISCOMP_COMPONENT Hypo71


#include <seiscomp3/core/system.h>
#include <seiscomp3/logging/log.h>
#include <seiscomp3/core/strings.h>
#include <seiscomp3/datamodel/utils.h>
#include <seiscomp3/datamodel/comment.h>
#include <seiscomp3/math/geo.h>
#include <seiscomp3/math/vector3.h>
#include <seiscomp3/utils/files.h>

#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <cstdlib>
#include <math.h>

#include "configfile.h"
#include "hypo71.h"


#define IQ 2
#define IPUN 1
#define IPRN 1
#define KSORT ""


#define DEBUG
#define HYPOINFO


ADD_SC_PLUGIN("HYPO71 seismic hypocenter locator plugin", "IPGP <www.ipgp.fr>", 0, 1, 0)


using namespace std;
using namespace Seiscomp;
using namespace Seiscomp::Core;
using namespace Seiscomp::DataModel;
using namespace Seiscomp::Seismology;


REGISTER_LOCATOR(Hypo71, "Hypo71");

Hypo71::IDList Hypo71::_allowedParameters;


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Hypo71::Hypo71() {

	_name = "Hypo71";
	_publicIDPattern = "Hypo71.@time/%Y%m%d%H%M%S.%f@.@id@";
	_allowMissingStations = false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Hypo71::~Hypo71() {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Hypo71::init(const Config::Config& config) {

	bool result = true;
	_currentProfile = NULL;
	SEISCOMP_DEBUG("[plugin] [hypo71] -----------------------------------------------------------------");
	SEISCOMP_DEBUG("[plugin] [hypo71] |                    CONFIGURATION PARAMETERS                   |");
	SEISCOMP_DEBUG("[plugin] [hypo71] -----------------------------------------------------------------");

	try {
		_publicIDPattern = config.getString("hypo71.publicID");
	}
	catch ( ... ) {
		_publicIDPattern = "Hypo71.@time/%Y%m%d%H%M%S.%f@.@id@";
	}

	try {
		_logFile = config.getString("hypo71.logFile");
		SEISCOMP_DEBUG("[plugin] [hypo71] |   logFile            | %s", _logFile.c_str());
	}
	catch ( ... ) {
		_logFile = "HYPO71.LOG";
		SEISCOMP_ERROR("[plugin] [hypo71] |   logFile            | DEFAULT value: %s", _logFile.c_str());
	}

	try {
		_h71inputFile = config.getString("hypo71.inputFile");
		SEISCOMP_DEBUG("[plugin] [hypo71] | inputFile            | %s", _h71inputFile.c_str());
	}
	catch ( ... ) {
		_h71inputFile = "HYPO71.INP";
		SEISCOMP_ERROR("[plugin] [hypo71] | inputFile            | DEFAULT value: %s", _h71inputFile.c_str());
	}

	try {
		_h71outputFile = config.getString("hypo71.outputFile");
		SEISCOMP_DEBUG("[plugin] [hypo71] | outputFile           | %s", _h71outputFile.c_str());
	}
	catch ( ... ) {
		_h71outputFile = "HYPO71.PRT";
		SEISCOMP_ERROR("[plugin] [hypo71] | outputFile           | DEFAULT value: %s", _h71outputFile.c_str());
	}

	try {
		_controlFilePath = config.getString("hypo71.defaultControlFile");
		SEISCOMP_DEBUG("[plugin] [hypo71] | defaultControlFile   | %s", _controlFilePath.c_str());
	}
	catch ( ... ) {
		_controlFilePath = "";
		SEISCOMP_ERROR("[plugin] [hypo71] | defaultControlFile   | DEFAULT value: %s", _controlFilePath.c_str());
	}
	if ( !Util::fileExists(_controlFilePath) ) {
		SEISCOMP_ERROR("[plugin] [hypo71] | defaultControlFile   | %s does not exist", _controlFilePath.c_str());
		result = false;
	}

	try {
		_hypo71BinaryFile = config.getString("hypo71.hypo71BinaryFile");
		SEISCOMP_DEBUG("[plugin] [hypo71] | hypo71BinaryFile     | %s", _hypo71BinaryFile.c_str());
	}
	catch ( ... ) {
		SEISCOMP_ERROR("[plugin] [hypo71] | hypo71BinaryFile     | can't read value");
	}

	try {
		_hypo71ScriptFile = config.getString("hypo71.hypo71ScriptFile");
		SEISCOMP_DEBUG("[plugin] [hypo71] | hypo71ScriptFile     | %s", _hypo71ScriptFile.c_str());
	}
	catch ( ... ) {
		SEISCOMP_ERROR("[plugin] [hypo71] | hypo71ScriptFile     | can't read value");
	}

	if ( !Util::fileExists(_hypo71ScriptFile) ) {
		SEISCOMP_ERROR("[plugin] [hypo71] | hypo71ScriptFile     | %s does not exist", _hypo71ScriptFile.c_str());
		result = false;
	}
	if ( !Util::fileExists(_hypo71BinaryFile) ) {
		SEISCOMP_ERROR("[plugin] [hypo71] | hypo71BinaryFile     | %s does not exist", _hypo71BinaryFile.c_str());
		result = false;
	}

	_profileNames.clear();
	try {
		_profileNames = config.getStrings("hypo71.profiles");
	}
	catch ( ... ) {
		SEISCOMP_ERROR("[plugin] [hypo71] CONFIGURATION FILE IS NOT CORRECTLY IMPLEMENTED!!");
	}

	for (IDList::iterator it = _profileNames.begin(); it != _profileNames.end();
	        ) {

		Profile prof;
		string prefix = string("hypo71.profile.") + *it + ".";

		prof.name = *it;
		SEISCOMP_DEBUG("[plugin] [hypo71] | NEW PROFILE !!       | %s", prof.name.c_str());
		try {
			prof.earthModelID = config.getString(prefix + "earthModelID");
			SEISCOMP_DEBUG("[plugin] [hypo71] |   earthModelID       | %s", prof.earthModelID.c_str());
		}
		catch ( ... ) {
			SEISCOMP_ERROR("[plugin] [hypo71] |   earthModelID       | can't read value");
		}

		try {
			prof.methodID = config.getString(prefix + "methodID");
			SEISCOMP_DEBUG("[plugin] [hypo71] |   methodID           | %s", prof.methodID.c_str());
		}
		catch ( ... ) {
			prof.methodID = "hypo71";
			SEISCOMP_ERROR("[plugin] [hypo71] |   methodID           | DEFAULT value: %s", prof.methodID.c_str());
		}

		try {
			prof.controlFile = config.getString(prefix + "controlFile");
			SEISCOMP_DEBUG("[plugin] [hypo71] |   configFile         | %s", prof.controlFile.c_str());
		}
		catch ( ... ) {
			SEISCOMP_ERROR("[plugin] [hypo71] |   configFile         | can't read value");
		}


		if ( prof.controlFile.empty() )
			prof.controlFile = _controlFilePath;

		if ( !Util::fileExists(prof.controlFile) ) {
			SEISCOMP_ERROR("[plugin] [hypo71] |   configFile         | file %s does not exist", prof.controlFile.c_str());
			it = _profileNames.erase(it);
			result = false;
			continue;
		}

		_profiles.push_back(prof);
		++it;
	}

	_profileNames.insert(_profileNames.begin(), "SELECT PROFILE");

	SEISCOMP_DEBUG("[plugin] [hypo71] -----------------------------------------------------------------");
	updateProfile("");

	return result;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Hypo71::IDList Hypo71::parameters() const {
	return _allowedParameters;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string Hypo71::parameter(const string& name) const {

	ParameterMap::const_iterator it = _parameters.find(name);
	if ( it != _parameters.end() )
		return it->second;

	return "";
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Hypo71::setParameter(const string& name, const string& value) {

	ParameterMap::iterator it = _parameters.find(name);
	if ( it == _parameters.end() )
		return false;
	it->second = value;

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string Hypo71::geoSituation(double d, int card) {

	std::string value;
	switch ( card ) {
		case 0:
			if ( d < .0 ) {
				value = "S";
			}
			else {
				value = "N";
			}
			break;
		case 1:
			if ( d < .0 ) {
				value = "W";
			}
			else {
				value = "E";
			}
			break;
		default:
			value = "undefined";
			break;
	}

	return value;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string Hypo71::stripSpace(string &str) {

	for (unsigned int i = 0; i < str.length(); i++)
		if ( str[i] == ' ' ) {
			str.erase(i, 1);
			i--;
		}

	return str;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Hypo71::stringIsOfType(std::string str, int type) {

	int i;
	float f;
	switch ( type ) {
		case 0:
			if ( sscanf(str.c_str(), "%f", &f) != 0 )
				return true;
			break;
		case 1:
			if ( sscanf(str.c_str(), "%d", &i) != 0 )
				return true;
			break;
		default:
			return false;
			break;
	}

	return false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int Hypo71::toInt(std::string str) {

	int value;
	std::istringstream iss(str);
	iss >> value;

	return value;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string Hypo71::DecimalToSexagesimal(double value, int polarity) {

	std::string convertedValue;
	std::string card;
	std::string II;
	std::string AA;
	std::string CC;

	double I = floor(abs(value));
	double D = abs(value) - I;
	double M = D * 60;
	double A = floor(M);
	double B = M - A;
	double C = B * 60;

	if ( value < 0 ) {
		switch ( polarity ) {
			//! latitude
			case 0:
				card = "S";
				break;
				//! longitude
			case 1:
				card = "W";
				break;
			default:
				card = "-";
				break;
		}
	}
	else {
		switch ( polarity ) {
			//! latitude
			case 0:
				card = "N";
				break;
				//! longitude
			case 1:
				card = "E";
				break;
			default:
				card = "-";
				break;
		}
	}

	//! -------------------------------------------
	//! | this part concerns float points & cie   |
	//! | > maybe a little raw but does the trick |
	//! -------------------------------------------
	if ( toString(I).size() != 2 ) {
		II = "0" + toString(I);
	}
	else
		II = toString(I);

	if ( toString(A).size() != 2 ) {
		AA = "0" + toString(A);
	}
	else
		AA = toString(A);

	if ( toString((int) C).size() != 2 ) {
		CC = toString((int) C) + "0";
	}
	else
		CC = toString((int) C);

	convertedValue = II + AA + "." + CC + card;

	return convertedValue;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string Hypo71::getSituation(double value, int type) {

	std::string pos;
	if ( value < 0 ) {
		switch ( type ) {
			//! latitude
			case 0:
				pos = "S";
				break;
				//! longitude
			case 1:
				pos = "W";
				break;
			default:
				pos = "-";
				break;
		}
	}
	else {
		switch ( type ) {
			//! latitude
			case 0:
				pos = "N";
				break;
				//! longitude
			case 1:
				pos = "E";
				break;
			default:
				pos = "-";
				break;
		}
	}

	return pos;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string Hypo71::DecimalToSexagesimalHypo71(double value) {

	std::string output;
	char dec[5];
	double i = floor(abs(value));
	double d = (abs(value) - i) * 60;
	d = floor(d * 100) / 100;
	sprintf(dec, "%#05.2f", d);
	output = toString(i) + toString(dec);

	return output;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string Hypo71::SexagesimalToDecimal(double deg, double min,
                                         double sec, std::string polarity) {

	std::string convertedValue;
	double A = sec / 60;
	double minSec = (min + A) / 60;
	double B = abs(deg) + minSec;

	if ( deg < 0 )
		B = -B;

	if ( (polarity == "S") or (polarity == "W") or (polarity == "s") or (polarity == "w") ) {
		B = -B;
	}
	convertedValue = toString(B);

	return convertedValue;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string Hypo71::getMinDeg(double value) {

	std::string min;
	double I = floor(abs(value));
	double D = abs(value) - I;
	double M = D * 60;
	double A = floor(M);
	double B = M - A;
	double C = B * 60;
	min = toString(C);

	return min;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string Hypo71::SexagesimalToDecimalHypo71(double deg, double min,
                                               std::string pol) {

	std::string value;
	double x = min / 60;
	double y = abs(deg) + x;
	if ( (pol == "S") or (pol == "W") or (pol == "s") or (pol == "w") ) {
		y = -y;
	}
	value = toString(y);

	return value;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Hypo71::IDList Hypo71::profiles() const {
	return _profileNames;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Hypo71::setProfile(const string &name) {

	if ( find(_profileNames.begin(), _profileNames.end(), name) == _profileNames.end() )
		return;
	else
		updateProfile(name);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Hypo71::stringExplode(std::string str, std::string separator,
                           std::vector<string>* results) {

	int found;
	found = str.find_first_of(separator);
	while ( found != (signed) string::npos ) {
		if ( found > 0 )
			results->push_back(str.substr(0, found));
		str = str.substr(found + 1);
		found = str.find_first_of(separator);
	}

	if ( str.length() > 0 )
		results->push_back(str);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int Hypo71::capabilities() const {
	return FixedDepth | DistanceCutOff;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
double Hypo71::toDouble(std::string s) {

	std::stringstream ss(s);
	double f;
	ss >> f;

	return f;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
double Hypo71::getTimeValue(PickList& pickList, std::string name,
                            std::string phase, unsigned int rtype) {

	double time = -1.;
	for (PickList::iterator i = pickList.begin(); i != pickList.end(); ++i) {
		Pick* p = i->first.get();
		if ( (toString(p->phaseHint().code()) == phase) && toString(p->waveformID().stationCode()) == name ) {
			switch ( rtype ) {
				case 0:
					time = p->time().value();
					break;
				case 1:
					time = toDouble(p->time().value().toString("%H"));
					break;
				case 2:
					time = toDouble(p->time().value().toString("%M"));
					break;
				case 3:
					time = toDouble(p->time().value().toString("%S.%f"));
					break;
				default:
					time = p->time().value();
					break;
			}
		}
	}

	return time;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string Hypo71::formatString(std::string toFormat, unsigned int nb,
                                 int pos, std::string sender) throw (Core::GeneralException) {

	if ( toFormat.size() > nb ) {
		SEISCOMP_ERROR("[plugin] [hypo71] Can't format string %s : length(%d) > length(%d) [sender: %s]",
		    toFormat.c_str(), (int ) toFormat.size(), nb, sender.c_str());
		throw LocatorException("SeisComP internal error\nTrying to fit " +
		        toString(toFormat.size()) + " chars into a " + toString(nb) + " chars variable");
	}

	unsigned int count;
	std::string blank;
	count = nb - toFormat.size();
	if ( count > 0 ) {
		while ( blank.size() < count ) {
			blank += " ";
		}
		switch ( pos ) {
			//! add blank space before
			case 0:
				toFormat = blank + toFormat;
				break;
				//! add blank space after
			case 1:
				toFormat += blank;
				break;
			default:
				SEISCOMP_ERROR("[plugin] [hypo71] formatString >> wrong position passed as argument");
				break;
		}
	}

	return toFormat;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int Hypo71::getH71Weight(PickList& pickList, std::string name,
                         std::string type, double max) {

	int weight;
	double upper, lower;
	std::string pickID;
	for (PickList::iterator it = pickList.begin(); it != pickList.end(); ++it) {
		Pick *pick = it->first.get();
		if ( (toString(pick->phaseHint().code().c_str()) == type) && (toString(pick->waveformID().stationCode()) == name) ) {
			pickID = pick->publicID();
			upper = pick->time().upperUncertainty();
			lower = pick->time().lowerUncertainty();
		}
	}
	if ( pickID != "" ) {
		DataModel::Origin *o = o->Find(_currentOriginID);
		if ( o != NULL ) {
			for (size_t i = 0; i < o->arrivalCount(); ++i) {
				if ( o->arrival(i)->pickID() == pickID ) {
					if ( o->arrival(i)->weight() != 1.0 ) {
						weight = 4;
					}
					if ( o->arrival(i)->weight() == 1.0 ) {
						weight = (int) round((3 / max) * (upper + lower));
					}
				}
			}
		}
		else
			SEISCOMP_DEBUG("[plugin] [hypo71] Can't find origin associated to id %s", _currentOriginID.c_str());
	}

	return weight;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Origin * Hypo71::locate(PickList& pickList) throw (Core::GeneralException) {

	std::ofstream log(_logFile.c_str(), ios::app);

	/* time log's buffer */
	time_t rawtime;
	struct tm * timeinfo;
	char head[80];
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(head, 80, "%F %H:%M:%S [log] ", timeinfo);

	std::string calculatedZTR;
	if ( _usingFixedDepth == true )
		calculatedZTR = toString(round(_fixedDepth));
	else
		calculatedZTR = Hypo71::getZTR(pickList);

	std::vector<string> Tvelocity;
	std::vector<string> Tdepth;

	//! not really used but may be useful to store debug log
	_lastWarning = "";

	if ( pickList.empty() )
		SEISCOMP_ERROR("H71 - Empty observation set");

	if ( _currentProfile == NULL )
		throw GeneralException("No profile selected! please do so !");

	if ( _usingFixedDepth == true )
		SEISCOMP_DEBUG("[plugin] [hypo71] Fixed depth value; %s", toString(round(_fixedDepth)).c_str());
	else
		SEISCOMP_DEBUG("[plugin] [hypo71] Proceeding to localization without fixed depth");


	//! cutting off stations which previous station-hypocenter distance is higher than distanceCutOff parameter value
	if ( _enableDistanceCutOff == true )
		SEISCOMP_DEBUG("[plugin] [hypo71] Cutoff distance value; %s", toString(_distanceCutOff).c_str());
	else
		SEISCOMP_DEBUG("[plugin] [hypo71] Proceeding to localization without distance cut off");


	/*----------------------------------*
	 | DEFAULT CONFIGURATION PARAMETERS |
	 |            (default.hypo71.conf) |
	 *----------------------------------*/
	defaultResetList dRL;
	defaultControlCard dCC;
	defaultInstructionCard dIC;
	ConfigFile dConfig(_controlFilePath);

	//! reset list values
	dConfig.readInto(dRL.test01, "TEST(01)");
	dConfig.readInto(dRL.test02, "TEST(02)");
	dConfig.readInto(dRL.test03, "TEST(03)");
	dConfig.readInto(dRL.test04, "TEST(04)");
	dConfig.readInto(dRL.test05, "TEST(05)");
	dConfig.readInto(dRL.test06, "TEST(06)");
	dConfig.readInto(dRL.test07, "TEST(07)");
	dConfig.readInto(dRL.test08, "TEST(08)");
	dConfig.readInto(dRL.test09, "TEST(09)");
	dConfig.readInto(dRL.test10, "TEST(10)");
	dConfig.readInto(dRL.test11, "TEST(11)");
	dConfig.readInto(dRL.test12, "TEST(12)");
	dConfig.readInto(dRL.test13, "TEST(13)");
	dConfig.readInto(dRL.test15, "TEST(15)");
	dConfig.readInto(dRL.test20, "TEST(20)");

	//! control card values
	dConfig.readInto(dCC.ztr, "ZTR");
	dConfig.readInto(dCC.xnear, "XNEAR");
	dConfig.readInto(dCC.xfar, "XFAR");
	dConfig.readInto(dCC.pos, "POS");
	dConfig.readInto(dCC.kfm, "KFM");
	dCC.kaz = "";
	dCC.kms = "0";
	dCC.imag = "";
	dCC.ir = "";
	dCC.ksel = "";
	string _iq = "2";
	dConfig.readInto(dCC.iq, "IQ", _iq);
	string _ipun = "1";
	dConfig.readInto(dCC.ipun, "IPUN", _ipun);
	string _iprn = "0";
	dConfig.readInto(dCC.iprn, "IPRN", _iprn);
	dCC.ksort = "";

	//! default instruction card values !
	dConfig.readInto(dIC.ipro, "IPRO");
	dConfig.readInto(dIC.knst, "KNST");
	dConfig.readInto(dIC.inst, "INST");
	dConfig.readInto(dIC.zres, "ZRES");
	dIC.ipro = "";
	dIC.zres = "";


	/*---------------------------------*
	 | CUSTOM CONFIGURATION PARAMETERS |
	 |                (profile.*.conf) |
	 *---------------------------------*/
	customResetList cRL;
	customControlCard cCC;
	customInstructionCard cIC;
	ConfigFile pConfig(_currentProfile->controlFile);


	//! reset list values
	pConfig.readInto(cRL.test01, "TEST(01)", dRL.test01);
	pConfig.readInto(cRL.test02, "TEST(02)", dRL.test02);
	pConfig.readInto(cRL.test03, "TEST(03)", dRL.test03);
	pConfig.readInto(cRL.test04, "TEST(04)", dRL.test04);
	pConfig.readInto(cRL.test05, "TEST(05)", dRL.test05);
	pConfig.readInto(cRL.test06, "TEST(06)", dRL.test06);
	pConfig.readInto(cRL.test07, "TEST(07)", dRL.test07);
	pConfig.readInto(cRL.test08, "TEST(08)", dRL.test08);
	pConfig.readInto(cRL.test09, "TEST(09)", dRL.test09);
	pConfig.readInto(cRL.test10, "TEST(10)", dRL.test10);
	pConfig.readInto(cRL.test11, "TEST(11)", dRL.test11);
	pConfig.readInto(cRL.test12, "TEST(12)", dRL.test12);
	pConfig.readInto(cRL.test13, "TEST(13)", dRL.test13);
	pConfig.readInto(cRL.test15, "TEST(15)", dRL.test15);
	pConfig.readInto(cRL.test20, "TEST(20)", dRL.test20);


	//! crustal model list
	pConfig.readInto(velocityModel, "CRUSTAL_VELOCITY_MODEL");
	Hypo71::stringExplode(velocityModel, ",", &Tvelocity);

	pConfig.readInto(depthModel, "CRUSTAL_DEPTH_MODEL");
	Hypo71::stringExplode(depthModel, ",", &Tdepth);

	if ( Tvelocity.size() != Tdepth.size() ) {
		SEISCOMP_ERROR("[plugin] [hypo71] Crustal velocity / depth doesn't match up, please correct that.");
		throw LocatorException("ERROR! Velocity model and Depth model doesn't match up in configuration file");
	}

	//! control card parameters
	pConfig.readInto(cCC.ztr, "ZTR", dCC.ztr);
	if ( _usingFixedDepth == true )
		cCC.ztr = toString(round(_fixedDepth));
	pConfig.readInto(cCC.xnear, "XNEAR", dCC.xnear);

	if ( _enableDistanceCutOff == true )
		cCC.xfar = toString(_distanceCutOff);
	else
		pConfig.readInto(cCC.xfar, "XFAR", dCC.xfar);

	pConfig.readInto(cCC.pos, "POS", dCC.pos);
	pConfig.readInto(cCC.kfm, "KFM", dCC.kfm);
	pConfig.readInto(cCC.ktest, "KTEST", dCC.ktest);
	pConfig.readInto(cCC.kaz, "KAZ", dCC.kaz);
	pConfig.readInto(cCC.kms, "KMS", dCC.kms);
	pConfig.readInto(cCC.iprn, "IPRN", dCC.iprn);
	cCC.imag = "";
	cCC.ir = "";
	cCC.ksel = "";
	cCC.iq = dCC.iq;
	cCC.ipun = dCC.ipun;
	cCC.ksort = dCC.ksort;


	//! available only on custom configuration file
	//! should we use last origin lat/lon to reprocess origin localization ?
	if ( pConfig.read("DISABLE_LAST_LOC", true) ) {
		_useLastOriginAsReference = false;
		cCC.lat1 = "";
		cCC.lat2 = "";
		cCC.lon1 = "";
		cCC.lon2 = "";
	}
	else {
		/**
		 * Really strange:
		 * HYPO71 doesn't take last lat/lon cardinal situation (NSEW)
		 **/
		_useLastOriginAsReference = true;
		cCC.lat1 = Hypo71::DecimalToSexagesimalHypo71(_oLastLatitude).substr(0, 2);
		cCC.lat2 = Hypo71::DecimalToSexagesimalHypo71(_oLastLatitude).substr(2, 7);
		cCC.lon1 = Hypo71::DecimalToSexagesimalHypo71(_oLastLongitude).substr(0, 2);
		cCC.lon2 = Hypo71::DecimalToSexagesimalHypo71(_oLastLongitude).substr(2, 7);
	}

	//! instruction card parameters            !
	pConfig.readInto(cIC.knst, "KNST", dIC.knst);
	pConfig.readInto(cIC.inst, "INST", dIC.inst);
	cIC.ipro = dIC.ipro;
	cIC.zres = dIC.zres;

	//! opening the HYPO71.INP file who will be used throughout the entire function
	std::ofstream h71in(_h71inputFile.c_str());

	/*-----------------------*/
	/*| HYPO71 HEADING CARD |*/
	/*-----------------------*/
	h71in << "HEAD" << endl;

	/*---------------------*/
	/*| HYPO71 RESET LIST |*/
	/*---------------------*/
	h71in << "RESET TEST(01)=" << cRL.test01 << endl;
	h71in << "RESET TEST(02)=" << cRL.test02 << endl;
	h71in << "RESET TEST(03)=" << cRL.test03 << endl;
	h71in << "RESET TEST(04)=" << cRL.test04 << endl;
	h71in << "RESET TEST(05)=" << cRL.test05 << endl;
	h71in << "RESET TEST(06)=" << cRL.test06 << endl;
	h71in << "RESET TEST(10)=" << cRL.test10 << endl;
	h71in << "RESET TEST(11)=" << cRL.test11 << endl;
	h71in << "RESET TEST(12)=" << cRL.test12 << endl;
	h71in << "RESET TEST(13)=" << cRL.test13 << endl;
	h71in << "RESET TEST(15)=" << cRL.test15 << endl;
	h71in << "RESET TEST(20)=" << cRL.test20 << endl;

	//! deactivated -> md plugin purpose
	//! h71in << "RESET TEST(07)=" << cRL.test07 << endl;
	//! h71in << "RESET TEST(08)=" << cRL.test08 << endl;
	//! h71in << "RESET TEST(09)=" << cRL.test09 << endl;

	/*---------------------*/
	/*| HYPO71 BLANK CARD |*/
	/*---------------------*/
	h71in << Hypo71::formatString("", 1, 0, "blank card") << endl;


	PickList usedPicks;
	std::string lastStation;

	//! origin informations to re-use later
	std::string oYear;

	//! creates observation buffer
	for (PickList::iterator it = pickList.begin(); it != pickList.end(); ++it) {

		Pick *pick = it->first.get();
		SensorLocation *sloc = getSensorLocation(pick);

		if ( sloc == NULL ) {
			if ( _allowMissingStations ) {
				// Append a new line to the warning message
				if ( !_lastWarning.empty() )
					_lastWarning += "\n";
				_lastWarning += "Sensor location ";
				_lastWarning += pick->waveformID().networkCode() + "." +
				        pick->waveformID().stationCode() + "." +
				        pick->waveformID().locationCode() +
				        " is not available and ignored";
				continue;
			}
			else {
				throw StationNotFoundException((string("Station ") +
				        pick->waveformID().networkCode() + "." +
				        pick->waveformID().stationCode() +
				        " not found.\nPlease deselect it to process localization.").c_str());
			}
		}

		usedPicks.push_back(*it);
		oYear = pick->time().value().toString("%Y");


#ifdef PDEBUG
		SEISCOMP_DEBUG("[plugin] [hypo71] --------------------------------");
		SEISCOMP_DEBUG("[plugin] [hypo71] |    STATION PICK RECEPTION    |");
		SEISCOMP_DEBUG("[plugin] [hypo71] --------------------------------");
		SEISCOMP_DEBUG("[plugin] [hypo71] | Code name        | %s", pick->waveformID().stationCode().c_str());
		SEISCOMP_DEBUG("[plugin] [hypo71] | Latitude         | %s", toString(sloc->latitude()).c_str());
		SEISCOMP_DEBUG("[plugin] [hypo71] | Longitude        | %s", toString(sloc->longitude()).c_str());
		SEISCOMP_DEBUG("[plugin] [hypo71] | Elevation        | %s", toString(sloc->elevation()).c_str());
		SEISCOMP_DEBUG("[plugin] [hypo71] | Time value       | %s", pick->time().value().toString("%Y%m%d").c_str());
		SEISCOMP_DEBUG("[plugin] [hypo71] | Phase hint code  | %s", pick->phaseHint().code().c_str());
		SEISCOMP_DEBUG("[plugin] [hypo71] --------------------------------");
#endif


		if ( lastStation != toString(pick->waveformID().stationCode()) ) {
			/*------------------------------*/
			/*| HYPO71 STATION DELAY MODEL |*/
			/*------------------------------*/
			h71in
			//! default blank
			<< Hypo71::formatString("", 2, 0)

			//!  station name
			<< Hypo71::formatString(toString(pick->waveformID().stationCode()), 4, 1, "station name")

			//! station latitude's degree + station latitude's minute //! float 7.2
			<< Hypo71::DecimalToSexagesimalHypo71(sloc->latitude())

			//! hemispheric station situation N/S //! Alphanumeric 1
			<< Hypo71::formatString(Hypo71::getSituation(sloc->latitude(), 0), 1, 0, "hemispheric situation")

			//! blank space
			<< Hypo71::formatString("", 1, 0)

			//! station longitude's degree + station longitude's minute //! float 7.2
			<< Hypo71::DecimalToSexagesimalHypo71(sloc->longitude())

			//! hemispheric station situation E/W //! Alphanumeric 1
			<< Hypo71::formatString(Hypo71::getSituation(sloc->longitude(), 1), 1, 0, "hemispheric situation")

			//! station elevation //! integer 4
			<< Hypo71::formatString(toString((int) sloc->elevation()).c_str(), 4, 0, "station elevation")

			//! blank space
			<< Hypo71::formatString("", 1, 0)

			//! station delay //! float 5.2
			<< Hypo71::formatString("0.00", 5, 0)

			//! station correction for FMAG //! float 5.2
			<< Hypo71::formatString("", 5, 0)

			//! station correction for XMAG //! float 5.2
			<< Hypo71::formatString("", 5, 0)

			//! system number assigned to station //! integer 1
			//! 0 - WoodAndersion
			//! 1 - NCER Standard
			//! 2 - EV-17 and Develco
			//! 3 - HS-10 and Teledyne
			//! 4 - HS-10 and Develco
			//! 5 - L-4C and Develco
			//! 6 - L-4C and Teledyne
			//! 7 - L-4C and replcing HS-10 and Develco
			//! 8 - 10-day recorders
			<< Hypo71::formatString("", 1, 0)

			//! XMAG period //! float 5.2
			<< Hypo71::formatString("", 5, 0)

			//! XMAG calibration //! float 4.2
			<< Hypo71::formatString("", 4, 0)

			//! calibration indicator //! integer 1
			<< Hypo71::formatString("", 1, 0) << endl;

		}
		lastStation = toString(pick->waveformID().stationCode());
	}                    //! end for


	/*---------------------*/
	/*| HYPO71 BLANK CARD |*/
	/*---------------------*/
	h71in << Hypo71::formatString("", 1, 0) << endl;


	/*-----------------------------*/
	/*| HYPO71 CRUSTAL MODEL LIST |*/
	/*-----------------------------*/
	for (long i = 0; i < (long) Tvelocity.size(); i++) {
		h71in << formatString(stripSpace(Tvelocity.at(i)), 7, 0)
		      << formatString(stripSpace(Tdepth.at(i)), 7, 0) << endl;
	}

	/*---------------------*/
	/*| HYPO71 BLANK CARD |*/
	/*---------------------*/
	h71in << Hypo71::formatString("", 1, 0) << endl;



	/*----------------*/
	/*| CONTROL CARD |*/
	/*----------------*/
	h71in

	//! blank space
	<< Hypo71::formatString("", 1, 0)

	//! Trial focal depth in km //! float 5.0
	//<< Hypo71::formatString(cCC.ztr, 5, 0)
	<< Hypo71::formatString(calculatedZTR, 4, 0, "trial focal depth")

	//! distance in km from epicenter where the distance weighting is 1 //! float 5.0
	<< Hypo71::formatString(cCC.xnear, 5, 0, "xnear")

	//! distance in km from epicenter beyond which the distance weighting is 0 //! float 5.0
	<< Hypo71::formatString(cCC.xfar, 5, 0, "xfar")

	//! ratio of P-velocity to S-velocity //! float 5.2
	<< Hypo71::formatString(cCC.pos, 5, 0, "ratio P-S velocity")

	//! blank space
	<< Hypo71::formatString("", 4, 0)

	//! quality class of earthquake to be included in the summary of residuals //! integer 1
	<< Hypo71::formatString(cCC.iq, 1, 0, "quality of earthquake")

	//! blank space
	<< Hypo71::formatString("", 4, 0)

	//! indicator to check missing data //! integer 1
	<< Hypo71::formatString(cCC.kms, 1, 0, "kms")

	//! blank space
	<< Hypo71::formatString("", 3, 0)

	//! minimum number of first motion reading required before it is plotted integer 2
	<< Hypo71::formatString(cCC.kfm, 2, 0, "kfm")

	//! blank space
	<< Hypo71::formatString("", 4, 0)

	//! indicator for punched card //!  integer 1
	<< Hypo71::formatString(cCC.ipun, 1, 0, "ipun")

	//! blank space
	<< Hypo71::formatString("", 4, 0)

	//! method of selecting earthquake magnitude //! integer 1
	<< Hypo71::formatString(cCC.imag, 1, 0, "imag")

	//! blank space
	<< Hypo71::formatString("", 4, 0)

	//! number of new system response curves to be read in //! integer 1
	<< Hypo71::formatString(cCC.ir, 1, 0, "response curve")

	//! blank space
	<< Hypo71::formatString("", 4, 0)

	//! indicator for printed output //! integer 1
	<< Hypo71::formatString(cCC.iprn, 1, 0, "iprn")

	//! blank space
	<< Hypo71::formatString("", 1, 0)

	//! helps to determine if the solution is at the minimum RMS //! integer 1
	<< Hypo71::formatString(cCC.ktest, 1, 0, "ktest")

	//! helps to apply the azimuthal weight of stations //! integer 1
	<< Hypo71::formatString(cCC.kaz, 1, 0, "kaz")

	//! to sort the station by distance in the output file //! integer 1
	<< Hypo71::formatString(cCC.ksort, 1, 0, "ksort")

	//! printed output will start at a new page ? //! integer 1
	<< Hypo71::formatString(cCC.ksel, 1, 0, "ksel")

	//! blank space
	<< Hypo71::formatString("", 2, 0)

	//! degree portion of the trial hypocenter latitude //! integer 2
	<< Hypo71::formatString(cCC.lat1, 2, 0, "lat1")

	//! blank space
	<< Hypo71::formatString("", 1, 0)

	//! minute portion of the trial hypocenter latitude //! float 5.2
	<< Hypo71::formatString(cCC.lat2, 5, 0, "lat2")

	//! blank space
	<< Hypo71::formatString("", 1, 0)

	//! degree portion of the trial longitude //! integer 3
	<< Hypo71::formatString(cCC.lon1, 3, 0, "lon1")

	//! blank space
	<< Hypo71::formatString("", 1, 0)

	//! minute portion of the trial longitude //! float 5.2
	<< Hypo71::formatString(cCC.lon2, 5, 0, "lon2") << endl;




	/*---------------*/
	/*| PHASES LIST |*/
	/*---------------*/
	bool isPPhase = false;
	bool isSPhase = false;

	//! searching for First Arrival Station (FAS)
	bool foundFAS = false;
	double refTime = .0, refTimeSec, prevRefTime, prevRefTimeSec;
	int refTimeMin, prevRefTimeMin;
	std::string refTimeYear, refTimeMonth, refTimeDay, refTimeHour, refStation;
	std::string prevRefTimeYear, prevRefTimeMonth, prevRefTimeDay,
	        prevRefTimeHour, prevRefStation;

	//! uncertainty values
	double maxUncertainty = -1, minUncertainty = 100;
	std::string maxWeight = "0";
	std::string minWeight = "4";


	for (PickList::iterator i = pickList.begin(); i != pickList.end(); ++i) {
		Pick* p = i->first.get();

		double ctime = (double) p->time().value();
		if ( refTime == 0 )
			refTime = ctime;
		if ( toString(p->phaseHint().code().c_str()) == "P" ) {
			if ( ctime <= refTime ) {
				refTime = ctime;
				refTimeYear = p->time().value().toString("%Y");
				refTimeMonth = p->time().value().toString("%m");
				refTimeDay = p->time().value().toString("%d");
				refTimeHour = p->time().value().toString("%H");
				refTimeMin = toInt(p->time().value().toString("%M"));
				refTimeSec = toDouble(p->time().value().toString("%S.%f"));
				refStation = toString(p->waveformID().stationCode());
				foundFAS = true;
			}
			else {
				prevRefTime = ctime;
				prevRefTimeYear = p->time().value().toString("%Y");
				prevRefTimeMonth = p->time().value().toString("%m");
				prevRefTimeDay = p->time().value().toString("%d");
				prevRefTimeHour = p->time().value().toString("%H");
				prevRefTimeMin = toInt(p->time().value().toString("%M"));
				prevRefTimeSec = toDouble(p->time().value().toString("%S.%f"));
				prevRefStation = toString(p->waveformID().stationCode());
			}
		}

		double upper = .0;
		double lower = .0;
		try {
			if ( p->time().upperUncertainty() != .0 ) {
				upper = p->time().upperUncertainty();
			}
			if ( p->time().lowerUncertainty() != .0 ) {
				lower = p->time().lowerUncertainty();
			}
			if ( (lower + upper) > maxUncertainty )
				maxUncertainty = lower + upper;
			if ( (lower + upper) < minUncertainty )
				minUncertainty = lower + upper;
		}
		catch ( ... ) {
		}

	}


	if ( foundFAS != true ) {
		refTime = prevRefTime;
		refTimeYear = prevRefTimeYear;
		refTimeMonth = prevRefTimeMonth;
		refTimeDay = prevRefTimeDay;
		refTimeHour = prevRefTimeHour;
		refTimeMin = prevRefTimeMin;
		refTimeSec = prevRefTimeSec;
		refStation = prevRefStation;
	}

	if ( refStation != "" ) {
		SEISCOMP_DEBUG("[plugin] [hypo71] First arrival station detected: %s - %s-%s-%s %s:%d:%.2f",
		    refStation.c_str(), refTimeYear.c_str(), refTimeMonth.c_str(), refTimeDay.c_str(),
		    refTimeHour.c_str(), refTimeMin, refTimeSec);
	}
	else
		throw LocatorException("SeisComP internal error\nFAS station detection failed");

	int sharedTime = ((int) (refTime / 3600) * 3600);
	std::string oDate = refTimeYear.substr(2, 2) + refTimeMonth + refTimeDay;
	std::string h71PWeight;
	std::string h71SWeight;
	std::vector<string> Tstation;

	SEISCOMP_DEBUG("Debug - RefDate: %s / time: %d", oDate.c_str(), sharedTime);

	//!---------------------------------
	//! retrieves phases from iterator !
	//!---------------------------------
	for (PickList::iterator it = pickList.begin(); it != pickList.end(); ++it) {
		Pick* pick = it->first.get();

		std::string pMinute;
		std::string pSec;
		std::string sSec;
		std::string staName = toString(pick->waveformID().stationCode());

		char buffer[10];
		double pmin, psec, ssec;

		SEISCOMP_DEBUG("        station: %s/%s", pick->waveformID().networkCode().c_str(),
		    pick->waveformID().stationCode().c_str());


		/*
		 * pick->phaseHint().code() = P
		 * we're just gonna look for an eventual S-phase
		 * if there is none, we only add the P-phase
		 * that's it.
		 */
		if ( toString(pick->phaseHint().code().c_str()) == "P" ) {
			bool isIntegrated = false;
			for (unsigned long x = 0; x < Tstation.size(); x++) {
				if ( Tstation[x] == stripSpace(staName) ) {
					isIntegrated = true;
				}
			}
			if ( isIntegrated == false ) {
				Tstation.push_back(stripSpace(staName));
				isPPhase = true;
				pmin = sharedTime + ((int) (((double) pick->time().value() - sharedTime) / 60) * 60);
				double newmin = (pmin / 60) - (int) (sharedTime / 3600) * 60;
				pMinute = toString((int) newmin);

				psec = Hypo71::getTimeValue(pickList, staName, "P", 0) - pmin;
				if ( psec > 99.99 )
					sprintf(buffer, "%#03.1f", ssec);
				else
					sprintf(buffer, "%#02.2f", psec);
				pSec = toString(buffer);

				try {
					h71PWeight = toString(getH71Weight(pickList, staName, "P", maxUncertainty));
				}
				catch ( ... ) {
					h71PWeight = maxWeight;
				}

				ssec = Hypo71::getTimeValue(pickList, staName, "S", 0) - pmin;
				if ( ssec > 0. ) {
					//! if ssec > 99.99 then it won't fit into a F5.2 so we convert it into a F5.1
					if ( ssec > 99.99 )
						sprintf(buffer, "%#03.1f", ssec);
					else
						sprintf(buffer, "%#02.2f", ssec);
					sSec = toString(buffer);
					isSPhase = true;
					try {
						h71SWeight = toString(getH71Weight(pickList, staName, "S", maxUncertainty));
					}
					catch ( ... ) {
						h71SWeight = maxWeight;
					}
				}
			}
		}


		//! writing down P-phase with S-phase
		if ( isPPhase == true && isSPhase == true ) {

			SEISCOMP_DEBUG("[plugin] [hypo71] applying %s's P phase weight of %s",
			    staName.c_str(), h71PWeight.c_str());
			SEISCOMP_DEBUG("[plugin] [hypo71] applying %s's S phase weight of %s",
			    staName.c_str(), h71SWeight.c_str());

			h71in
			//! station name //! alphanumeric 4
			//<< Hypo71::formatString(toString(pick->waveformID().stationCode()), 4, 1)
			<< Hypo71::formatString(staName, 4, 1)

			//! description of onset of P-arrival //! alphanumeric 1
			<< Hypo71::formatString("E", 1, 0)

			//! 'P' to denote P-arrival //! alphanumeric 1
			<< "P"

			//! first motion direction of P-arrival //! alphanumeric 1
			<< Hypo71::formatString("", 1, 0)

			//! weight assigned to P-arrival //! float 1.0
			//<< Hypo71::formatString("3", 1, 0)
			<< Hypo71::formatString(h71PWeight, 1, 0)

			//! blank space between 8-10
			<< Hypo71::formatString("", 1, 0)

			//! year, month and day of P-arrival //! integer 6
			<< Hypo71::formatString(oDate, 6, 0)

			//! hour of P-arrival //! integer 2
			<< Hypo71::formatString(refTimeHour, 2, 0)

			//! minute of P-arrival //! integer 2
			<< Hypo71::formatString(pMinute, 2, 0)

			//! second of P-arrival //! float 5.2
			<< Hypo71::formatString(pSec, 5, 0)

			//! blank space between 24-32
			<< Hypo71::formatString("", 7, 0)

			//! second of S-arrival //! float of 5.2
			<< Hypo71::formatString(sSec, 5, 0)

			//! description of onset S-arrival //! alphanumeric 1
			<< Hypo71::formatString("E", 1, 0)

			//! 'S' to denote S-arrival //! alphanumeric 1
			<< "S"

			//! first motion direction //! alphanumeric 1
			<< Hypo71::formatString("", 1, 0)

			//! weight assigned to S-arrival //! float 1.0
			//<< Hypo71::formatString("3", 1, 0)
			<< Hypo71::formatString(h71SWeight, 1, 0)

			//! maximum peak-to-peak amplitude in mm //! float 4.0
			<< Hypo71::formatString("", 4, 0)

			//! period of the maximum amplitude in sec //! float 3.2
			<< Hypo71::formatString("", 3, 0)

			//! normally not used except as note in next item //! float 4.1
			<< Hypo71::formatString("", 4, 0)

			//! blank space between 54-59
			<< Hypo71::formatString("", 5, 0)

			//! peak-to-peak amplitude of 10 microvolts calibration signal in mm //! float 4.1
			<< Hypo71::formatString("", 4, 0)

			//! remark for this phase card //! alphanumeric 3
			<< Hypo71::formatString("", 3, 0)

			//! time correction in sec //! float 5.2
			<< Hypo71::formatString("", 5, 0) << endl;

			isSPhase = false;
			isPPhase = false;
			staName = "";

		}                    //! end writing down P-phase with S-phase


		//! writing down P-phase without S-phase
		if ( isPPhase == true && isSPhase == false ) {
			SEISCOMP_DEBUG("[plugin] [hypo71] applying %s's P phase weight of %s",
			    staName.c_str(), h71PWeight.c_str());

			h71in
			//! station name //! alphanumeric 4
			//<< Hypo71::formatString(toString(pick->waveformID().stationCode()), 4, 1)
			<< Hypo71::formatString(staName, 4, 1)

			//! description of onset of P-arrival //! alphanumeric 1
			<< Hypo71::formatString("E", 1, 0)

			//! 'P' to denote P-arrival //! alphanumeric 1
			<< "P"

			//! first motion direction of P-arrival //! alphanumeric 1
			<< Hypo71::formatString("", 1, 0)

			//! weight assigned to P-arrival //! float 1.0
			//<< Hypo71::formatString("3", 1, 0)
			<< Hypo71::formatString(h71PWeight, 1, 0)

			//! blank space between 8-10
			<< Hypo71::formatString("", 1, 0)

			//! year, month and day of P-arrival //! integer 6
			<< Hypo71::formatString(oDate, 6, 0)

			//! hour of P-arrival //! integer 2
			<< Hypo71::formatString(refTimeHour, 2, 0)

			//! minute of P-arrival //! integer 2
			<< Hypo71::formatString(pMinute, 2, 0)

			//! second of P-arrival //! float 5.2
			<< Hypo71::formatString(pSec, 5, 0)

			//! blank space between 24-32
			<< Hypo71::formatString("", 7, 0)

			//! second of S-arrival //! float of 5.2
			<< Hypo71::formatString(sSec, 5, 0)

			//! description of onset S-arrival //! alphanumeric 1
			<< Hypo71::formatString("", 1, 0)

			//! 'S' to denote S-arrival //! alphanumeric 1
			<< Hypo71::formatString("", 1, 0)

			//! first motion direction //! alphanumeric 1
			<< Hypo71::formatString("", 1, 0)

			//! weight assigned to S-arrival //! float 1.0
			<< Hypo71::formatString("", 1, 0)

			//! maximum peak-to-peak amplitude in mm //! float 4.0
			<< Hypo71::formatString("", 4, 0)

			//! period of the maximum amplitude in sec //! float 3.2
			<< Hypo71::formatString("", 3, 0)

			//! normally not used except as note in next item //! float 4.1
			<< Hypo71::formatString("", 4, 0)

			//! blank space between 54-59
			<< Hypo71::formatString("", 5, 0)

			//! peak-to-peak amplitude of 10 microvolts calibration signal in mm //! float 4.1
			<< Hypo71::formatString("", 4, 0)

			//! remark for this phase card //! alphanumeric 3
			<< Hypo71::formatString("", 3, 0)

			//! time correction in sec //! float 5.2
			<< Hypo71::formatString("", 5, 0) << endl;
			isSPhase = false;
			isPPhase = false;
			staName = "";
		}                    //! end writing down P-phase without S-phase

	} //! end for



	//? ##----------------##
	//? # instruction card #
	//? ##----------------##
	h71in
	//! blank space between 1-4
	<< Hypo71::formatString("", 4, 0)

	//! IPRO //! alphanumeric 4
	<< Hypo71::formatString(cIC.ipro, 4, 0)

	//! blank space between 8-18
	<< Hypo71::formatString("", 9, 0)

	//! Use S Data ? //! integer 1
	<< Hypo71::formatString(cIC.knst, 1, 0)

	//! Fix depth ? //! integer 1
	<< Hypo71::formatString(cIC.inst, 1, 0)

	//! trial focal-depth //! float 5.2
	<< Hypo71::formatString(cIC.zres, 5, 0) << endl;



	//! let's close the generated HYPO71.INP
	h71in.close();


	//!-----------------------------------------------------------
	//! now we need to call the HYPO71 binary file.              !
	//! Hypo71 seismic localization should take about .5 seconds !
	//!-----------------------------------------------------------
	system(_hypo71ScriptFile.c_str());

	/*-----------------------------------*
	 | READING BACK GENERATED HYPO71.PRT |
	 *-----------------------------------*/
	if ( _h71outputFile.empty() ) {
		throw LocatorException("ERROR! Processing stream failed\nOutput file is empty");
	}


	//!  New Origin creation
	DataModel::Origin *origin = DataModel::Origin::Create();
	std::string originLine = "DATEORIGINLAT";
	std::string stationListStart = "STNDISTAZMAINPRMKHRMNP-SECTPOBSTPCALDLY/H1P-RESP-WTAMXPRXCALXKXMAGRMKFMPFMAGSRMKS-SECTSOBSS-RESS-WTDT";
	std::string stationListEnd = "1*****CLASS:ABCDTOTAL*****";

	int loop = 0;
	std::string lineContent;
	std::ifstream ifile(_h71outputFile.c_str());
	std::string line;

	//! recovering N/S E/W from origin line
	std::string latSit;
	std::string lonSit;

	int lineNumber = 1;
	int event = -1;
	int staStart;
	int staEnd;

	//  ---------------------------------------
	//  First output file reading iteration
	//  We locate the part of interest
	//  ---------------------------------------
	while ( ifile.good() ) {
		getline(ifile, line);

		//! trimming white spaces from file line
		std::string trimmedLine = stripSpace(line);

		//! if trimmed line and origineLine match we've got our origin line
		if ( trimmedLine.substr(0, 13) == originLine ) {
			event = lineNumber;
			latSit = line.substr(13, 1);
			lonSit = line.substr(18, 1);
		}

		//! if trimmed line and stationListStart match we've got our station list start point
		if ( trimmedLine == stationListStart ) {
			staStart = lineNumber;
		}

		//! if trimmed line and stationListEnd match we've got our station list end point
		if ( trimmedLine == stationListEnd ) {
			staEnd = lineNumber - 1;
		}

		lineNumber++;
	}
	ifile.close();


	//! If event variable value is equal to her initialization value, the outputted
	//! file is not correct and we must stop the localization process
	if ( event == -1 ) {
		throw LocatorException("ERROR! The generated output file is not correct\nPlease review your configuration");
	}

	std::ifstream file(_h71outputFile.c_str());

	//! pick indexer
	int idx = 1;


	Time ot;
	DataModel::CreationInfo ci;
	DataModel::OriginQuality oq;

	double rms = 0;
	double hrms;
	//int phaseAssocCount = 0;
	int usedAssocCount = 0;
	std::vector<double> Tdist;
	std::vector<double> Tazi;
	int depthPhaseCount = 0;
	std::string gap;

	//  ----------------------------------------
	//  Second output file reading iteration
	//  We just get back the H71 run information
	//  and store them into SeisComP3 objects
	//  -----------------------------------------
	while ( file.good() ) {
		getline(file, lineContent);

		if ( loop == event ) {

			std::string year, month, day, tHour, tMin, tSec, latDeg, latMin,
			        lonDeg, lonMin, depth, orms, erh, erz, nbStations, tLat,
			        tLon, quality;

			try {
				year = lineContent.substr(1, 2);
				year = stripSpace(year);
			}
			catch ( std::exception &e ) {
				SEISCOMP_ERROR("Year exception: %s", e.what());
			}
			try {
				month = lineContent.substr(3, 2);
				month = stripSpace(month);
			}
			catch ( std::exception &e ) {
				SEISCOMP_ERROR("Month exception: %s", e.what());
			}
			try {
				day = lineContent.substr(5, 2);
				day = stripSpace(day);
			}
			catch ( std::exception &e ) {
				SEISCOMP_ERROR("day exception: %s", e.what());
			}
			try {
				tHour = lineContent.substr(8, 2);
				tHour = stripSpace(tHour);
			}
			catch ( std::exception &e ) {
				SEISCOMP_ERROR("thour exception: %s", e.what());
			}
			try {
				tMin = lineContent.substr(10, 2);
				tMin = stripSpace(tMin);
			}
			catch ( std::exception &e ) {
				SEISCOMP_ERROR("tmin exception: %s", e.what());
			}
			try {
				tSec = lineContent.substr(13, 5);
				tSec = stripSpace(tSec);
			}
			catch ( std::exception &e ) {
				SEISCOMP_ERROR("tsec exception: %s", e.what());
			}
			try {
				latDeg = lineContent.substr(18, 3);
				latDeg = stripSpace(latDeg);
			}
			catch ( std::exception &e ) {
				SEISCOMP_ERROR("latdeg exception: %s", e.what());
			}
			try {
				latMin = lineContent.substr(22, 5);
				latMin = stripSpace(latMin);
			}
			catch ( std::exception &e ) {
				SEISCOMP_ERROR("latmin exception: %s", e.what());
			}
			try {
				lonDeg = lineContent.substr(28, 3);
				lonDeg = stripSpace(lonDeg);
			}
			catch ( std::exception &e ) {
				SEISCOMP_ERROR("londeg exception: %s", e.what());
			}
			try {
				lonMin = lineContent.substr(32, 5);
				lonMin = stripSpace(lonMin);
			}
			catch ( std::exception &e ) {
				SEISCOMP_ERROR("lonmin exception: %s", e.what());
			}
			try {
				depth = lineContent.substr(37, 7);
				depth = stripSpace(depth);
			}
			catch ( std::exception &e ) {
				SEISCOMP_ERROR("depth exception: %s", e.what());
			}
			try {
				gap = lineContent.substr(57, 4);
				gap = stripSpace(gap);
			}
			catch ( std::exception &e ) {
				SEISCOMP_ERROR("gap exception: %s", e.what());
			}
			try {
				orms = lineContent.substr(63, 5);
				orms = stripSpace(orms);
				hrms = toDouble(orms);
			}
			catch ( std::exception &e ) {
				SEISCOMP_ERROR("orms exception: %s", e.what());
			}
			try {
				erh = lineContent.substr(68, 5);
				erh = stripSpace(erh);
			}
			catch ( std::exception &e ) {
				SEISCOMP_ERROR("erh exception: %s", e.what());
			}
			try {
				erz = lineContent.substr(73, 5);
				erz = stripSpace(erz);
			}
			catch ( std::exception &e ) {
				SEISCOMP_ERROR("erz exception: %s", e.what());
			}

			try {
				quality = lineContent.substr(78, 2);
				quality = stripSpace(quality);
			}
			catch ( std::exception &e ) {
				SEISCOMP_ERROR("quality exception: %s", e.what());
			}

			try {
				nbStations = lineContent.substr(51, 3);
				nbStations = stripSpace(nbStations);
			}
			catch ( std::exception &e ) {
				SEISCOMP_ERROR("nbStations exception: %s", e.what());
			}

			tLat = Hypo71::SexagesimalToDecimalHypo71(toDouble(latDeg), toDouble(latMin), latSit);
			tLon = Hypo71::SexagesimalToDecimalHypo71(toDouble(lonDeg), toDouble(lonMin), lonSit);

			//! setting origin time
			ot.set(toInt(oYear), toInt(month), toInt(day), toInt(tHour), toInt(tMin),
			    (int) floor(toDouble(tSec)), (int) ((toDouble(tSec) - floor(toDouble(tSec))) * 1.0E6));
			origin->setTime(ot);

			//! setting origin creation time
			ci.setCreationTime(Core::Time().gmt());
			origin->setCreationInfo(ci);


			/*! Dealing with ERH
			 *  if ERH has been calculated, we convert it into SC3 correction
			 *  otherwise we put it to NONE
			 */
			if ( erh.compare("") != 0 ) {
				double corr = toDouble(erh) / sqrt(2);
				origin->setLatitude(RealQuantity(toDouble(tLat), corr, None, None, None));
				origin->setLongitude(RealQuantity(toDouble(tLon), corr, None, None, None));
			}
			else {
				origin->setLatitude(RealQuantity(toDouble(tLat), None, None, None, None));
				origin->setLongitude(RealQuantity(toDouble(tLon), None, None, None, None));
				SEISCOMP_DEBUG("No ERH calculated by Hypo71, putting default NONE value");
			}


			/*! Dealing with ERZ
			 *  if ERZ has been calculated, we convert it into SC3 correction
			 *  otherwise we put it to NONE
			 */
			if ( erz.compare("") != 0 ) {
				origin->setDepth(RealQuantity(toDouble(depth), toDouble(erz), None, None, None));
			}
			else {
				origin->setDepth(RealQuantity(toDouble(depth), None, None, None, None));
				SEISCOMP_DEBUG("No ERZ calculated by Hypo71, putting default NONE value");
			}
			_usingFixedDepth = false;


			//! Set origin quality
			oq.setUsedStationCount(toInt(nbStations));
			oq.setAzimuthalGap(toDouble(gap));
			oq.setGroundTruthLevel(quality);

#ifdef HYPOINFO
			SEISCOMP_DEBUG("[plugin] [hypo71] --------------------------------------------");
			SEISCOMP_DEBUG("[plugin] [hypo71] |     HYPO71 NEW HYPOCENTER ESTIMATION     |");
			SEISCOMP_DEBUG("[plugin] [hypo71] --------------------------------------------");
			SEISCOMP_DEBUG("[plugin] [hypo71] | Date              | %s-%s-%s", year.c_str(), month.c_str(), day.c_str());
			SEISCOMP_DEBUG("[plugin] [hypo71] | Time              | %s:%s:%s", tHour.c_str(), tMin.c_str(), tSec.c_str());
			SEISCOMP_DEBUG("[plugin] [hypo71] | Decimal latitude  | %s°", tLat.c_str());
			SEISCOMP_DEBUG("[plugin] [hypo71] | Decimal longitude | %s°", tLon.c_str());
			SEISCOMP_DEBUG("[plugin] [hypo71] | Depth             | %skm", depth.c_str());
			SEISCOMP_DEBUG("[plugin] [hypo71] | RMS               | %s", orms.c_str());
			SEISCOMP_DEBUG("[plugin] [hypo71] | Azimuthal GAP     | %s°", gap.c_str());
			SEISCOMP_DEBUG("[plugin] [hypo71] | Phases            | %s", nbStations.c_str());
			if ( stringIsOfType(erh, 0) == true )
				SEISCOMP_DEBUG("[plugin] [hypo71] | ERH               | %s", erh.c_str());
			if ( stringIsOfType(erz, 0) == true )
				SEISCOMP_DEBUG("[plugin] [hypo71] | ERZ               | %s", erz.c_str());
			SEISCOMP_DEBUG("[plugin] [hypo71] --------------------------------------------");

#endif
			//log << head << "*---------------------------------------*" << endl;
			log << head << "|          FINAL RUN RESEULTS           |" << endl;
			log << head << "*---------------------------------------*" << endl;
			log << head << "|       DATE: " << year << "-" << month << "-" << day << endl;
			log << head << "|       TIME: " << tHour << ":" << tMin << ":" << tSec << endl;
			log << head << "|   LATITUDE: " << tLat << endl;
			log << head << "|  LONGITUDE: " << tLon << endl;
			log << head << "|      DEPTH: " << depth << endl;
			log << head << "|        RMS: " << orms << endl;
			log << head << "|    AZ. GAP: " << gap << endl;
			if ( stringIsOfType(erh, 0) == true )
				log << head << "|        ERH: " << erh << endl;
			if ( stringIsOfType(erz, 0) == true )
				log << head << "|        ERZ: " << erz << endl;
			log << head << "*---------------------------------------*" << endl;


		} // loop==event



		if ( (loop >= staStart) && (loop < staEnd) ) {


			std::string staName, dist, azimuth, takeOffAngle, hour, minute,
			        psec, pres, pwt, ssec, sres, swt;
			try {
				//! string 4 - station name
				staName = lineContent.substr(1, 4);
				staName = stripSpace(staName);
			}
			catch ( std::exception& e ) {
				SEISCOMP_ERROR("staname: %s", e.what());
			}
			try {
				//! float 6 - station distance to epicenter
				dist = lineContent.substr(5, 6);
				dist = stripSpace(dist);
			}
			catch ( std::exception& e ) {
				SEISCOMP_ERROR("dist; %s", e.what());
			}
			try {
				//! integer 4 - station azimuth
				azimuth = lineContent.substr(11, 4);
				azimuth = stripSpace(azimuth);
			}
			catch ( std::exception& e ) {
				SEISCOMP_ERROR("azimuth: %s", e.what());
			}
			try {
				//! int 3 - AIN (Take Off Angle)
				takeOffAngle = lineContent.substr(15, 4);
				takeOffAngle = stripSpace(takeOffAngle);
			}
			catch ( std::exception& e ) {
				SEISCOMP_ERROR("takeoffangle: %s", e.what());
			}
			try {
				//! integer 2 - Arrival Hour
				hour = lineContent.substr(25, 2);
				hour = stripSpace(hour);
			}
			catch ( std::exception& e ) {
				SEISCOMP_ERROR("hour: %s", e.what());
			}
			try {
				//! integer 2 - Arrival Minute
				minute = lineContent.substr(27, 2);
				minute = stripSpace(minute);
			}
			catch ( std::exception& e ) {
				SEISCOMP_ERROR("minute: %s", e.what());
			}
			try {
				//! float 5 - P arrival seconds
				psec = lineContent.substr(30, 5);
				psec = stripSpace(psec);
			}
			catch ( std::exception& e ) {
				SEISCOMP_ERROR("psec: %s", e.what());
			}
			try {
				//! float 6 - P residuals
				pres = lineContent.substr(53, 6);
				pres = stripSpace(pres);
			}
			catch ( std::exception& e ) {
				SEISCOMP_ERROR("pres: %s", e.what());
			}
			try {
				//! float 5 - P weight
				pwt = lineContent.substr(61, 4);
				pwt = stripSpace(pwt);
			}
			catch ( std::exception& e ) {
				SEISCOMP_ERROR("pwt: %s", e.what());
			}
			try {
				//! float 6 - S arrival seconds
				ssec = lineContent.substr(103, 6);
				ssec = stripSpace(ssec);
			}
			catch ( std::exception& e ) {
				SEISCOMP_ERROR("ssec: %s", e.what());
			}
			try {
				//! float 5 - S residuals
				sres = lineContent.substr(115, 6);
				sres = stripSpace(sres);
			}
			catch ( std::exception& e ) {

				SEISCOMP_ERROR("sres: %s", e.what());
			}
			try {
				//! float 4 - S weight
				swt = lineContent.substr(123, 4);
				swt = stripSpace(swt);
			}
			catch ( std::exception& e ) {
				SEISCOMP_ERROR("swt: %s", e.what());
			}


			DataModel::CreationInfo aci;
			aci.setCreationTime(Core::Time::GMT());
			aci.setAuthor("Hypo71");
			aci.setModificationTime(Core::Time::GMT());

			/*!
			 * Associating new arrival with new origin.
			 * P.S.: the weight actually stays the same as the one in picklist
			 * since it is not the real weight but just intel about whether or not
			 * to use arrival (rms to high or some like that) [act as boolean 1 or 0]
			 */
			for (PickList::iterator it = pickList.begin(); it != pickList.end();
			        ++it) {

				PickPtr pick = it->first.get();
				double weight = it->second;

				if ( (pick->phaseHint().code() == "P")
				        && (pick->waveformID().stationCode() == stripSpace(staName)) ) {

					DataModel::ArrivalPtr pArrival = new DataModel::Arrival;
					pArrival->setPickID(pick->publicID());
					pArrival->setCreationInfo(aci);
					pArrival->setWeight(weight);
					pArrival->setDistance(Math::Geo::km2deg(toDouble(dist)));
					pArrival->setAzimuth(toInt(azimuth));
					pArrival->setPhase(Phase("P"));
					pArrival->setTimeResidual(toDouble(pres));
					pArrival->setTakeOffAngle(toDouble(takeOffAngle));

					if ( !origin->add(pArrival.get()) ) {
						origin->removeArrival(pArrival->index());
						origin->add(pArrival.get());
					}
				}

				if ( (pick->phaseHint().code() == "S")
				        && (pick->waveformID().stationCode() == stripSpace(staName)) ) {

					DataModel::ArrivalPtr sArrival = new DataModel::Arrival;
					sArrival->setPickID(pick->publicID());
					sArrival->setCreationInfo(aci);
					sArrival->setWeight(weight);
					sArrival->setDistance(Math::Geo::km2deg(toDouble(dist)));
					sArrival->setAzimuth(toInt(azimuth));
					sArrival->setPhase(Phase("S"));
					sArrival->setTimeResidual(toDouble(sres));
					sArrival->setTakeOffAngle(toDouble(takeOffAngle));

					if ( !origin->add(sArrival.get()) ) {
						origin->removeArrival(sArrival->index());
						origin->add(sArrival.get());
					}
				}
			}

			if ( toDouble(pwt) > 0.5 )
				depthPhaseCount++;

			idx++;

			if ( ssec != "" ) {

				if ( toDouble(swt) > 0.5 )
					depthPhaseCount++;
				idx++;

			}

			//! saving station arrival info
			Tdist.push_back(toDouble(dist));
			Tazi.push_back(toDouble(azimuth));
			rms += toDouble(pres) * toDouble(sres);
			++usedAssocCount;

#ifdef HYPOINFO
			SEISCOMP_DEBUG("[plugin] [hypo71] -----------------------------------");
			SEISCOMP_DEBUG("[plugin] [hypo71] |   HYPO71 STATION INFORMATIONS   |");
			SEISCOMP_DEBUG("[plugin] [hypo71] -----------------------------------");
			SEISCOMP_DEBUG("[plugin] [hypo71] | Name         | %s", staName.c_str());
			SEISCOMP_DEBUG("[plugin] [hypo71] | Ep. distance | %skm", dist.c_str());
			SEISCOMP_DEBUG("[plugin] [hypo71] | Azimuth      | %s°", azimuth.c_str());
			SEISCOMP_DEBUG("[plugin] [hypo71] | Hour         | %s:%s", hour.c_str(), minute.c_str());
			SEISCOMP_DEBUG("[plugin] [hypo71] | P-Sec        | %s", psec.c_str());
			SEISCOMP_DEBUG("[plugin] [hypo71] | P residuals  | %s", pres.c_str());
			SEISCOMP_DEBUG("[plugin] [hypo71] | P weight     | %s", pwt.c_str());
			if ( ssec != "" )
				SEISCOMP_DEBUG("[plugin] [hypo71] | S-Sec        | %s", ssec.c_str());
			if ( sres != "" )
				SEISCOMP_DEBUG("[plugin] [hypo71] | S residuals  | %s", sres.c_str());
			if ( swt != "" )
				SEISCOMP_DEBUG("[plugin] [hypo71] | S weight     | %s", swt.c_str());
			SEISCOMP_DEBUG("[plugin] [hypo71] -----------------------------------");
#endif


		} //  (loop >= staStart) && (loop < staEnd)

		loop++;
	} //  end of second file itereation
	file.close();



	origin->setEarthModelID(_currentProfile->earthModelID);
	origin->setMethodID("Hypo71");

	oq.setAssociatedPhaseCount(idx);
	oq.setUsedPhaseCount(idx);
	oq.setDepthPhaseCount(idx);
	oq.setStandardError(hrms);


	std::sort(Tazi.begin(), Tazi.end());
	Tazi.push_back(Tazi.front() + 360.);

	double azGap = 0.;
	if ( Tazi.size() > 2 )
		for (size_t i = 0; i < Tazi.size() - 1; i++)
			azGap = (Tazi[i + 1] - Tazi[i]) > azGap ? (Tazi[i + 1] - Tazi[i]) : azGap;

	std::sort(Tdist.begin(), Tdist.end());
	oq.setMinimumDistance(Math::Geo::km2deg(Tdist.front()));
	oq.setMaximumDistance(Math::Geo::km2deg(Tdist.back()));
	oq.setMedianDistance(Tdist[Tdist.size() / 2]);
	origin->setQuality(oq);

	log.close();

	return origin;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string Hypo71::getZTR(PickList& pickList) throw (Core::GeneralException) {

	std::vector<string> Tvelocity;
	std::vector<string> Tdepth;
	std::vector<string> Tztr;
	double minRMS = 10.;
	double minER = 10000.;
	std::string minDepth;
	char buf[10];
	std::ofstream log(_logFile.c_str(), ios::app);

	/* time log's buffer */
	time_t rawtime;
	struct tm * timeinfo;
	char head[80];
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(head, 80, "%F %H:%M:%S [log] ", timeinfo);

	log << endl;
	log << endl;
	log << head << "*---------------------------------------*" << endl;
	log << head << "|           NEW LOCALIZATION            |" << endl;
	log << head << "*---------------------------------------*" << endl;


	/*----------------------------------*
	 | DEFAULT CONFIGURATION PARAMETERS |
	 |            (default.hypo71.conf) |
	 *----------------------------------*/
	defaultResetList dRL;
	defaultControlCard dCC;
	defaultInstructionCard dIC;
	ConfigFile dConfig(_controlFilePath);

	//! reset list values
	dConfig.readInto(dRL.test01, "TEST(01)");
	dConfig.readInto(dRL.test02, "TEST(02)");
	dConfig.readInto(dRL.test03, "TEST(03)");
	dConfig.readInto(dRL.test04, "TEST(04)");
	dConfig.readInto(dRL.test05, "TEST(05)");
	dConfig.readInto(dRL.test06, "TEST(06)");
	dConfig.readInto(dRL.test07, "TEST(07)");
	dConfig.readInto(dRL.test08, "TEST(08)");
	dConfig.readInto(dRL.test09, "TEST(09)");
	dConfig.readInto(dRL.test10, "TEST(10)");
	dConfig.readInto(dRL.test11, "TEST(11)");
	dConfig.readInto(dRL.test12, "TEST(12)");
	dConfig.readInto(dRL.test13, "TEST(13)");
	dConfig.readInto(dRL.test15, "TEST(15)");
	dConfig.readInto(dRL.test20, "TEST(20)");

	//! control card values
	dConfig.readInto(dCC.ztr, "ZTR");
	dConfig.readInto(dCC.xnear, "XNEAR");
	dConfig.readInto(dCC.xfar, "XFAR");
	dConfig.readInto(dCC.pos, "POS");
	dConfig.readInto(dCC.kfm, "KFM");
	dCC.kaz = "";
	dCC.kms = "0";
	dCC.imag = "";
	dCC.ir = "";
	dCC.ksel = "";
	string _iq = "2";
	dConfig.readInto(dCC.iq, "IQ", _iq);
	string _ipun = "1";
	dConfig.readInto(dCC.ipun, "IPUN", _ipun);
	string _iprn = "0";
	dConfig.readInto(dCC.iprn, "IPRN", _iprn);
	dCC.ksort = "";

	//! default instruction card values !
	dConfig.readInto(dIC.ipro, "IPRO");
	dConfig.readInto(dIC.knst, "KNST");
	dConfig.readInto(dIC.inst, "INST");
	dConfig.readInto(dIC.zres, "ZRES");
	dIC.ipro = "";
	dIC.zres = "";


	/*---------------------------------*
	 | CUSTOM CONFIGURATION PARAMETERS |
	 |                (profile.*.conf) |
	 *---------------------------------*/
	customResetList cRL;
	customControlCard cCC;
	customInstructionCard cIC;
	ConfigFile pConfig(_currentProfile->controlFile);


	//! reset list values
	pConfig.readInto(cRL.test01, "TEST(01)", dRL.test01);
	pConfig.readInto(cRL.test02, "TEST(02)", dRL.test02);
	pConfig.readInto(cRL.test03, "TEST(03)", dRL.test03);
	pConfig.readInto(cRL.test04, "TEST(04)", dRL.test04);
	pConfig.readInto(cRL.test05, "TEST(05)", dRL.test05);
	pConfig.readInto(cRL.test06, "TEST(06)", dRL.test06);
	pConfig.readInto(cRL.test07, "TEST(07)", dRL.test07);
	pConfig.readInto(cRL.test08, "TEST(08)", dRL.test08);
	pConfig.readInto(cRL.test09, "TEST(09)", dRL.test09);
	pConfig.readInto(cRL.test10, "TEST(10)", dRL.test10);
	pConfig.readInto(cRL.test11, "TEST(11)", dRL.test11);
	pConfig.readInto(cRL.test12, "TEST(12)", dRL.test12);
	pConfig.readInto(cRL.test13, "TEST(13)", dRL.test13);
	pConfig.readInto(cRL.test15, "TEST(15)", dRL.test15);
	pConfig.readInto(cRL.test20, "TEST(20)", dRL.test20);


	//! crustal model list
	pConfig.readInto(velocityModel, "CRUSTAL_VELOCITY_MODEL");
	Hypo71::stringExplode(velocityModel, ",", &Tvelocity);

	pConfig.readInto(depthModel, "CRUSTAL_DEPTH_MODEL");
	Hypo71::stringExplode(depthModel, ",", &Tdepth);

	if ( Tvelocity.size() != Tdepth.size() ) {
		throw LocatorException("ERROR! Velocity model and Depth model doesn't match up in configuration file");
	}


	//! control card parameters
	pConfig.readInto(cCC.ztr, "ZTR", dCC.ztr);
	Hypo71::stringExplode(cCC.ztr, ",", &Tztr);
	if ( Tztr.size() < 2 ) {
		throw LocatorException("ERROR! Only one value has been set for ZTR trial depth\nIterations process needs at least two");
	}
	if ( _usingFixedDepth == true )
		cCC.ztr = toString(round(_fixedDepth));
	pConfig.readInto(cCC.xnear, "XNEAR", dCC.xnear);

	if ( _enableDistanceCutOff == true )
		cCC.xfar = toString(_distanceCutOff);
	else
		pConfig.readInto(cCC.xfar, "XFAR", dCC.xfar);

	pConfig.readInto(cCC.pos, "POS", dCC.pos);
	pConfig.readInto(cCC.kfm, "KFM", dCC.kfm);
	pConfig.readInto(cCC.ktest, "KTEST", dCC.ktest);
	pConfig.readInto(cCC.kaz, "KAZ", dCC.kaz);
	pConfig.readInto(cCC.kms, "KMS", dCC.kms);
	pConfig.readInto(cCC.iprn, "IPRN", dCC.iprn);
	cCC.imag = "";
	cCC.ir = "";
	cCC.ksel = "";
	cCC.iq = dCC.iq;
	cCC.ipun = dCC.ipun;
	cCC.ksort = dCC.ksort;

	// Available only on custom configuration file
	// Should we use last origin lat/lon to reprocess origin localization ?
	if ( pConfig.read("DISABLE_LAST_LOC", true) ) {
		_useLastOriginAsReference = false;
		cCC.lat1 = "";
		cCC.lat2 = "";
		cCC.lon1 = "";
		cCC.lon2 = "";
	}
	else {
		/**
		 * Really strange:
		 * HYPO71 doesn't take last lat/lon cardinal situation (NSEW)
		 **/
		_useLastOriginAsReference = true;
		cCC.lat1 = Hypo71::DecimalToSexagesimalHypo71(_oLastLatitude).substr(0, 2);
		cCC.lat2 = Hypo71::DecimalToSexagesimalHypo71(_oLastLatitude).substr(2, 7);
		cCC.lon1 = Hypo71::DecimalToSexagesimalHypo71(_oLastLongitude).substr(0, 2);
		cCC.lon2 = Hypo71::DecimalToSexagesimalHypo71(_oLastLongitude).substr(2, 7);
	}

	// Instruction card parameters            !
	pConfig.readInto(cIC.knst, "KNST", dIC.knst);
	pConfig.readInto(cIC.inst, "INST", dIC.inst);
	cIC.ipro = dIC.ipro;
	cIC.zres = dIC.zres;


	log << head << "| Picklist content" << endl;
	for (PickList::iterator i = pickList.begin(); i != pickList.end(); ++i) {
		Pick *p = i->first.get();
		log << head << "|  " << toString(p->phaseHint().code().c_str()) << " "
		    << p->waveformID().stationCode() << " " << p->time().value().toString("%H")
		    << ":" << toInt(p->time().value().toString("%M")) << ":"
		    << toDouble(p->time().value().toString("%S.%f")) << endl;
	}


	// Here start ZTR iterations
	// NB: we process several localizations using vector's Tztr values
	// and select the min(rms*sqrt(erh²+erz²)) as refrence
	for (long j = 0; j < (long) Tztr.size(); j++) {

		//! opening the HYPO71.INP file
		std::ofstream h71in(_h71inputFile.c_str());

		log << head << "*---------------------------------------*" << endl;
		log << head << "| " << "ITERATION " << j << " with ZTR fixed at "
		    << stripSpace(Tztr.at(j)) << "KM" << endl;

		/*-----------------------*/
		/*| HYPO71 HEADING CARD |*/
		/*-----------------------*/
		h71in << "HEAD" << endl;

		/*---------------------*/
		/*| HYPO71 RESET LIST |*/
		/*---------------------*/
		h71in << "RESET TEST(01)=" << cRL.test01 << endl;
		h71in << "RESET TEST(02)=" << cRL.test02 << endl;
		h71in << "RESET TEST(03)=" << cRL.test03 << endl;
		h71in << "RESET TEST(04)=" << cRL.test04 << endl;
		h71in << "RESET TEST(05)=" << cRL.test05 << endl;
		h71in << "RESET TEST(06)=" << cRL.test06 << endl;
		h71in << "RESET TEST(10)=" << cRL.test10 << endl;
		h71in << "RESET TEST(11)=" << cRL.test11 << endl;
		h71in << "RESET TEST(12)=" << cRL.test12 << endl;
		h71in << "RESET TEST(13)=" << cRL.test13 << endl;
		h71in << "RESET TEST(15)=" << cRL.test15 << endl;
		h71in << "RESET TEST(20)=" << cRL.test20 << endl;


		/*---------------------*/
		/*| HYPO71 BLANK CARD |*/
		/*---------------------*/
		h71in << Hypo71::formatString("", 1, 0) << endl;

		PickList usedPicks;
		std::string lastStation;

		// Origin information to re-use later
		std::string oYear;

		// Creates observation buffer
		for (PickList::iterator it = pickList.begin(); it != pickList.end();
		        ++it) {

			Pick* pick = it->first.get();
			SensorLocation* sloc = getSensorLocation(pick);
			usedPicks.push_back(*it);

			oYear = pick->time().value().toString("%Y");


			if ( lastStation != toString(pick->waveformID().stationCode()) ) {
				/*------------------------------*/
				/*| HYPO71 STATION DELAY MODEL |*/
				/*------------------------------*/
				h71in
				//! default blank
				<< Hypo71::formatString("", 2, 0)
				//!  station name
				<< Hypo71::formatString(toString(pick->waveformID().stationCode()), 4, 1, "station name")
				//! station latitude's degree + station latitude's minute //! float 7.2
				<< Hypo71::DecimalToSexagesimalHypo71(sloc->latitude())
				//! hemispheric station situation N/S //! Alphanumeric 1
				<< Hypo71::formatString(Hypo71::getSituation(sloc->latitude(), 0), 1, 0, "hemispheric situation")
				//! blank space
				<< Hypo71::formatString("", 1, 0)
				//! station longitude's degree + station longitude's minute //! float 7.2
				<< Hypo71::DecimalToSexagesimalHypo71(sloc->longitude())
				//! hemispheric station situation E/W //! Alphanumeric 1
				<< Hypo71::formatString(Hypo71::getSituation(sloc->longitude(), 1), 1, 0, "hemispheric situation")
				//! station elevation //! integer 4
				<< Hypo71::formatString(toString((int) sloc->elevation()).c_str(), 4, 0, "station elevation")
				//! blank space
				<< Hypo71::formatString("", 1, 0)
				//! station delay //! float 5.2
				<< Hypo71::formatString("0.00", 5, 0)
				//! station correction for FMAG //! float 5.2
				<< Hypo71::formatString("", 5, 0)
				//! station correction for XMAG //! float 5.2
				<< Hypo71::formatString("", 5, 0)
				//! system number assigned to station //! integer 1
				<< Hypo71::formatString("", 1, 0)
				//! XMAG period //! float 5.2
				<< Hypo71::formatString("", 5, 0)
				//! XMAG calibration //! float 4.2
				<< Hypo71::formatString("", 4, 0)
				//! calibration indicator //! integer 1
				<< Hypo71::formatString("", 1, 0) << endl;

			}
			lastStation = toString(pick->waveformID().stationCode());
		}    //! end for

		/*---------------------*/
		/*| HYPO71 BLANK CARD |*/
		/*---------------------*/
		h71in << Hypo71::formatString("", 1, 0) << endl;

		for (long i = 0; i < (long) Tvelocity.size(); i++) {
			h71in << formatString(stripSpace(Tvelocity.at(i)), 7, 0, "HYPO BLANK CARD")
			      << formatString(stripSpace(Tdepth.at(i)), 7, 0, "HYPO BLANK CARD") << endl;
		}

		/*---------------------*/
		/*| HYPO71 BLANK CARD |*/
		/*---------------------*/
		h71in << Hypo71::formatString("", 1, 0) << endl;


		//? ##------------##
		//? # control card #
		//? ##------------##
		h71in
		//! blank card
		<< Hypo71::formatString("", 1, 0)
		//! Trial focal depth in km //! float 5.0
		<< Hypo71::formatString(stripSpace(Tztr.at(j)), 4, 0, "trial focal depth")
		//! distance in km from epicenter where the distance weighting is 1 //! float 5.0
		<< Hypo71::formatString(cCC.xnear, 5, 0, "xnear")
		//! distance in km from epicenter beyond which the distance weighting is 0 //! float 5.0
		<< Hypo71::formatString(cCC.xfar, 5, 0, "xfar")
		//! ratio of P-velocity to S-velocity //! float 5.2
		<< Hypo71::formatString(cCC.pos, 5, 0, "ratio P-velocity to S-velocity")
		//! blank space
		<< Hypo71::formatString("", 4, 0)
		//! quality class of earthquake to be included in the summary of residuals //! integer 1
		<< Hypo71::formatString(cCC.iq, 1, 0, "class or earthquake")
		//! blank space
		<< Hypo71::formatString("", 4, 0)
		//! indicator to check missing data //! integer 1
		<< Hypo71::formatString(cCC.kms, 1, 0, "kms")
		//! blank space
		<< Hypo71::formatString("", 3, 0)
		//! minimum number of first motion reading required before it is plotted integer 2
		<< Hypo71::formatString(cCC.kfm, 2, 0, "kfm")
		//! blank space
		<< Hypo71::formatString("", 4, 0)
		//! indicator for punched card //!  integer 1
		<< Hypo71::formatString(cCC.ipun, 1, 0, "ipun")
		//! blank space
		<< Hypo71::formatString("", 4, 0)
		//! method of selecting earthquake magnitude //! integer 1
		<< Hypo71::formatString(cCC.imag, 1, 0, "imag")
		//! blank space
		<< Hypo71::formatString("", 4, 0)
		//! number of new system response curves to be read in //! integer 1
		<< Hypo71::formatString(cCC.ir, 1, 0, "response curve")
		//! blank space
		<< Hypo71::formatString("", 4, 0)
		//! indicator for printed output //! integer 1
		<< Hypo71::formatString(cCC.iprn, 1, 0, "iprn")
		//! blank space
		<< Hypo71::formatString("", 1, 0)
		//! helps to determine if the solution is at the minimum RMS //! integer 1
		<< Hypo71::formatString(cCC.ktest, 1, 0, "ktest")
		//! helps to apply the azimuthal weight of stations //! integer 1
		<< Hypo71::formatString(cCC.kaz, 1, 0, "kaz")
		//! to sort the station by distance in the output file //! integer 1
		<< Hypo71::formatString(cCC.ksort, 1, 0, "ksort")
		//! printed output will start at a new page ? //! integer 1
		<< Hypo71::formatString(cCC.ksel, 1, 0, "ksel")
		//! blank space
		<< Hypo71::formatString("", 2, 0)
		//! degree portion of the trial hypocenter latitude //! integer 2
		<< Hypo71::formatString(cCC.lat1, 2, 0, "lat1")
		//! blank space
		<< Hypo71::formatString("", 1, 0)
		//! minute portion of the trial hypocenter latitude //! float 5.2
		<< Hypo71::formatString(cCC.lat2, 5, 0, "lat2")
		//! blank space
		<< Hypo71::formatString("", 1, 0)
		//! degree portion of the trial longitude //! integer 3
		<< Hypo71::formatString(cCC.lon1, 3, 0, "lon1")
		//! blank space
		<< Hypo71::formatString("", 1, 0)
		//! minute portion of the trial longitude //! float 5.2
		<< Hypo71::formatString(cCC.lon2, 5, 0, "lon2") << endl;

		//? ##----------##
		//? # phase list #
		//? ##----------##
		std::string prevStaName;
		bool isPPhase = false;
		bool isSPhase = false;


		// Searching for First Arrival Station (FAS) variables
		bool foundFAS = false;
		double refTime = .0, prevRefTime;
		std::string refTimeYear, refTimeMonth, refTimeDay, refTimeHour,
		        refStation;
		std::string prevRefTimeYear, prevRefTimeMonth, prevRefTimeDay,
		        prevRefTimeHour, prevRefStation;

		// Uncertainty values
		double maxUncertainty = -1, minUncertainty = 100;
		std::string maxWeight = "0";
		std::string minWeight = "4";


		for (PickList::iterator i = pickList.begin(); i != pickList.end();
		        ++i) {
			Pick *p = i->first.get();
			double ctime = (double) p->time().value();

			if ( refTime == 0 )
				refTime = ctime;

			if ( toString(p->phaseHint().code().c_str()) == "P" ) {
				if ( ctime <= refTime ) {
					refTime = ctime;
					refTimeYear = p->time().value().toString("%Y");
					refTimeMonth = p->time().value().toString("%m");
					refTimeDay = p->time().value().toString("%d");
					refTimeHour = p->time().value().toString("%H");
					refStation = toString(p->waveformID().stationCode());
					foundFAS = true;
				}
				else {
					prevRefTime = ctime;
					prevRefTimeYear = p->time().value().toString("%Y");
					prevRefTimeMonth = p->time().value().toString("%m");
					prevRefTimeDay = p->time().value().toString("%d");
					prevRefTimeHour = p->time().value().toString("%H");
					prevRefStation = toString(p->waveformID().stationCode());
				}
			}

			double upper = .0;
			double lower = .0;
			try {
				if ( p->time().upperUncertainty() != .0 ) {
					upper = p->time().upperUncertainty();
				}
				if ( p->time().lowerUncertainty() != .0 ) {
					lower = p->time().lowerUncertainty();
				}
				if ( (lower + upper) > maxUncertainty )
					maxUncertainty = lower + upper;
				if ( (lower + upper) < minUncertainty )
					minUncertainty = lower + upper;
			}
			catch ( ... ) {
			}
		}

		if ( foundFAS != true ) {
			refTime = prevRefTime;
			refTimeYear = prevRefTimeYear;
			refTimeMonth = prevRefTimeMonth;
			refTimeDay = prevRefTimeDay;
			refTimeHour = prevRefTimeHour;
			refStation = prevRefStation;
		}


		if ( refStation == "" )
			throw LocatorException("ERROR! ZTR iteration can't identify the FAS station");


		int sharedTime = ((int) (refTime / 3600) * 3600);
		std::string oDate = refTimeYear.substr(2, 2) + refTimeMonth + refTimeDay;
		std::string h71PWeight;
		std::string h71SWeight;
		std::vector<string> Tstation;


		// Retrieves phases from iterator
		for (PickList::iterator it = pickList.begin(); it != pickList.end();
		        ++it) {
			Pick* pick = it->first.get();

			std::string pMinute;
			std::string pSec;
			std::string sSec;
			std::string staName = toString(pick->waveformID().stationCode());
			char buffer[10];
			double pmin, psec, ssec;

			std::string tmpDate = pick->time().value().toString("%Y").substr(2, 2)
			        + pick->time().value().toString("%m") + pick->time().value().toString("%d");
			std::string tmpHour = pick->time().value().toString("%H");


			/*
			 * pick->phaseHint().code() = P
			 * we're just gonna look for an eventual S-phase
			 * if there is none, we only add the P-phase
			 * that's it.
			 */
			if ( toString(pick->phaseHint().code().c_str()) == "P" ) {
				bool isIntegrated = false;
				for (unsigned long x = 0; x < Tstation.size(); x++) {
					if ( Tstation[x] == stripSpace(staName) ) {
						isIntegrated = true;
					}
				}
				if ( isIntegrated == false ) {
					Tstation.push_back(stripSpace(staName));
					isPPhase = true;
					pmin = sharedTime + ((int) (((double) pick->time().value() - sharedTime) / 60)) * 60;
					double newmin = pmin / 60 - (int) (sharedTime / 3600) * 60;
					pMinute = toString((int) newmin);

					psec = Hypo71::getTimeValue(pickList, staName, "P", 0) - pmin;
					if ( psec > 99.99 )
						sprintf(buffer, "%#03.1f", ssec);
					else
						sprintf(buffer, "%#02.2f", psec);
					pSec = toString(buffer);

					try {
						h71PWeight = toString(getH71Weight(pickList, staName, "P", maxUncertainty));
					}
					catch ( ... ) {
						h71PWeight = maxWeight;
					}

					ssec = Hypo71::getTimeValue(pickList, staName, "S", 0) - pmin;

					if ( ssec > 0. ) {
						//! if ssec > 99.99 then it won't fit into a F5.2 so we convert it into a F5.1
						if ( ssec > 99.99 )
							sprintf(buffer, "%#03.1f", ssec);
						else
							sprintf(buffer, "%#02.2f", ssec);
						sSec = toString(buffer);
						isSPhase = true;
						try {
							h71SWeight = toString(getH71Weight(pickList, staName, "S", maxUncertainty));
						}
						catch ( ... ) {
							h71SWeight = maxWeight;
						}
					}
				}
			}


			//! writing down P-phase with S-phase
			if ( isPPhase == true && isSPhase == true ) {

				if ( (int) sSec.size() > 5 )
					throw LocatorException("Hypo71 error!\n" + staName +
					        " S Phase is wrongly picked.\nHypo71 does not allow to fit " +
					        toString(sSec.size()) + " chars into 5 chars variable");

				h71in
				//! station name //! alphanumeric 4
				<< Hypo71::formatString(staName, 4, 1)
				//! description of onset of P-arrival //! alphanumeric 1
				<< Hypo71::formatString("E", 1, 0)
				//! 'P' to denote P-arrival //! alphanumeric 1
				<< "P"
				//! first motion direction of P-arrival //! alphanumeric 1
				<< Hypo71::formatString("", 1, 0)
				//! weight assigned to P-arrival //! float 1.0
				//<< Hypo71::formatString("3", 1, 0)
				<< Hypo71::formatString(h71PWeight, 1, 0)
				//! blank space between 8-10
				<< Hypo71::formatString("", 1, 0)
				//! year, month and day of P-arrival //! integer 6
				<< Hypo71::formatString(oDate, 6, 0, "ODATE")
				//! hour of P-arrival //! integer 2
				<< Hypo71::formatString(refTimeHour, 2, 0)
				//! minute of P-arrival //! integer 2
				<< Hypo71::formatString(pMinute, 2, 0)
				//! second of P-arrival //! float 5.2
				<< Hypo71::formatString(pSec, 5, 0, "PSEC")
				//! blank space between 24-32
				<< Hypo71::formatString("", 7, 0)
				//! second of S-arrival //! float of 5.2
				<< Hypo71::formatString(sSec, 5, 0, "SSEC")
				//! description of onset S-arrival //! alphanumeric 1
				<< Hypo71::formatString("E", 1, 0)
				//! 'S' to denote S-arrival //! alphanumeric 1
				<< "S"
				//! first motion direction //! alphanumeric 1
				<< Hypo71::formatString("", 1, 0)
				//! weight assigned to S-arrival //! float 1.0
				//<< Hypo71::formatString("3", 1, 0)
				<< Hypo71::formatString(h71SWeight, 1, 0)
				//! maximum peak-to-peak amplitude in mm //! float 4.0
				<< Hypo71::formatString("", 4, 0)
				//! period of the maximum amplitude in sec //! float 3.2
				<< Hypo71::formatString("", 3, 0)
				//! normally not used except as note in next item //! float 4.1
				<< Hypo71::formatString("", 4, 0)
				//! blank space between 54-59
				<< Hypo71::formatString("", 5, 0)
				//! peak-to-peak amplitude of 10 microvolts calibration signal in mm //! float 4.1
				<< Hypo71::formatString("", 4, 0)
				//! remark for this phase card //! alphanumeric 3
				<< Hypo71::formatString("", 3, 0)
				//! time correction in sec //! float 5.2
				<< Hypo71::formatString("", 5, 0) << endl;

				isSPhase = false;
				isPPhase = false;
				staName = "";
			}                        //! end writing down P-phase with S-phase


			//! writing down P-phase without S-phase
			if ( isPPhase == true && isSPhase == false ) {

				h71in
				//! station name //! alphanumeric 4
				//<< Hypo71::formatString(toString(pick->waveformID().stationCode()), 4, 1)
				<< Hypo71::formatString(staName, 4, 1)
				//! description of onset of P-arrival //! alphanumeric 1
				<< Hypo71::formatString("E", 1, 0)
				//! 'P' to denote P-arrival //! alphanumeric 1
				<< "P"
				//! first motion direction of P-arrival //! alphanumeric 1
				<< Hypo71::formatString("", 1, 0)
				//! weight assigned to P-arrival //! float 1.0
				//<< Hypo71::formatString("3", 1, 0)
				<< Hypo71::formatString(h71PWeight, 1, 0)
				//! blank space between 8-10
				<< Hypo71::formatString("", 1, 0)
				//! year, month and day of P-arrival //! integer 6
				<< Hypo71::formatString(oDate, 6, 0, "ODATE")
				//! hour of P-arrival //! integer 2
				<< Hypo71::formatString(refTimeHour, 2, 0)
				//! minute of P-arrival //! integer 2
				<< Hypo71::formatString(pMinute, 2, 0)
				//! second of P-arrival //! float 5.2
				<< Hypo71::formatString(pSec, 5, 0, "PSEC")
				//! blank space between 24-32
				<< Hypo71::formatString("", 7, 0)
				//! second of S-arrival //! float of 5.2
				<< Hypo71::formatString(sSec, 5, 0, "SSEC")
				//! description of onset S-arrival //! alphanumeric 1
				<< Hypo71::formatString("", 1, 0)
				//! 'S' to denote S-arrival //! alphanumeric 1
				<< Hypo71::formatString("", 1, 0)
				//! first motion direction //! alphanumeric 1
				<< Hypo71::formatString("", 1, 0)
				//! weight assigned to S-arrival //! float 1.0
				<< Hypo71::formatString("", 1, 0)
				//! maximum peak-to-peak amplitude in mm //! float 4.0
				<< Hypo71::formatString("", 4, 0)
				//! period of the maximum amplitude in sec //! float 3.2
				<< Hypo71::formatString("", 3, 0)
				//! normally not used except as note in next item //! float 4.1
				<< Hypo71::formatString("", 4, 0)
				//! blank space between 54-59
				<< Hypo71::formatString("", 5, 0)
				//! peak-to-peak amplitude of 10 microvolts calibration signal in mm //! float 4.1
				<< Hypo71::formatString("", 4, 0)
				//! remark for this phase card //! alphanumeric 3
				<< Hypo71::formatString("", 3, 0)
				//! time correction in sec //! float 5.2
				<< Hypo71::formatString("", 5, 0) << endl;
				isSPhase = false;
				isPPhase = false;
				staName = "";
			}                       //! end writing down P-phase without S-phase

		} //! end for

		//? ##----------------##
		//? # instruction card #
		//? ##----------------##
		h71in
		//! blank space between 1-4
		<< Hypo71::formatString("", 4, 0)
		//! IPRO //! alphanumeric 4
		<< Hypo71::formatString(cIC.ipro, 4, 0, "ipro")
		//! blank space between 8-18
		<< Hypo71::formatString("", 9, 0)
		//! Use S Data ? //! integer 1
		<< Hypo71::formatString(cIC.knst, 1, 0, "knst")
		//! Fix depth ? //! integer 1
		<< Hypo71::formatString(cIC.inst, 1, 0, "inst")
		//! trial focal-depth //! float 5.2
		<< Hypo71::formatString(cIC.zres, 5, 0, "zres") << endl;

		//! let's close the generated HYPO71.INP
		h71in.close();


		// Now we need to call the HYPO71 binary file.
		system(_hypo71ScriptFile.c_str());

		// READING BACK GENERATED HYPO71.PRT
		if ( _h71outputFile.empty() ) {
			throw LocatorException("HYPO71 computation error...");
		}
		else {
			std::string originLine = "DATEORIGINLAT";
			int loop = 0;
			std::string lineContent;
			std::ifstream ifile(_h71outputFile.c_str());
			std::string line;
			std::string cutoff = "*****INSUFFICIENTDATAFORLOCATINGTHISQUAKE:";

			// recovering N/S E/W from origin line
			std::string latSit;
			std::string lonSit;

			int lineNumber = 1;
			int event = -1;

			while ( ifile.good() ) {
				getline(ifile, line);
				// trimming white spaces from file line
				std::string trimmedLine = stripSpace(line);
				// if trimmed line and origineLine match we've got our origin line
				if ( trimmedLine.substr(0, 13) == originLine ) {
					event = lineNumber;
					latSit = line.substr(13, 1);
					lonSit = line.substr(18, 1);
				}

				if ( trimmedLine.substr(0, 42) == cutoff ) {
					throw LocatorException("ERROR! Epicentral distance out of XFAR range\nSet distance cutoff ON");
				}

				lineNumber++;
			}
			ifile.close();

			if ( event == -1 ) {
				SEISCOMP_ERROR("GetZTR failed");
				throw LocatorException("ERROR! Can't properly read output file\nPlease verify the output file");
			}

			std::ifstream file(_h71outputFile.c_str());

			double ER;
			while ( file.good() ) {
				getline(file, lineContent);
				if ( loop == event ) {
					try {
						string rms = lineContent.substr(63, 5);
						rms = stripSpace(rms);
						string erh = lineContent.substr(68, 5);
						erh = stripSpace(erh);
						string erz = lineContent.substr(73, 5);
						erz = stripSpace(erz);
						string depth = lineContent.substr(37, 7);
						depth = stripSpace(depth);

						string latDeg = lineContent.substr(18, 3);
						latDeg = stripSpace(latDeg);

						string latMin = lineContent.substr(22, 5);
						latMin = stripSpace(latMin);

						string lonDeg = lineContent.substr(28, 3);
						lonDeg = stripSpace(lonDeg);

						string lonMin = lineContent.substr(32, 5);
						lonMin = stripSpace(lonMin);

						string tLat = Hypo71::SexagesimalToDecimalHypo71(toDouble(latDeg), toDouble(latMin), latSit);
						string tLon = Hypo71::SexagesimalToDecimalHypo71(toDouble(lonDeg), toDouble(lonMin), lonSit);

						if ( (Hypo71::stringIsOfType(erh, 0) == true) & (Hypo71::stringIsOfType(erz, 0) == true) )
							ER = sqrt(pow(toDouble(erh), 2) + pow(toDouble(erz), 2));

						log << head << "|  RMS: " << formatString(rms, 10, 1) << "  LAT: " << tLat << endl;
						log << head << "|  ERH: " << formatString(erh, 10, 1) << "  LON: " << tLon << endl;
						log << head << "|  ERZ: " << formatString(erz, 10, 1) << "DEPTH: " << depth << "km" << endl;
						log << head << "|   ER: " << ER << endl;

						if ( (toDouble(rms) < minRMS) or (toDouble(rms) == minRMS) ) {
							if ( minDepth == "" )
								log << head << "|  ZTR set to " << depth << "km" << endl;
							else
								log << head << "|  " << minDepth << "km old ZTR is replaced by " << depth << "km" << endl;
							minDepth = depth;
							minRMS = toDouble(rms);
							if ( ER < minER ) {
								minER = ER;
								//log << head << depth << "km new ER value: " << ER << endl;
							}
						}
					}
					catch ( ... ) {
					}
				}

				loop++;
			}
			file.close();
		}
	}

	sprintf(buf, "%#04.0f", toDouble(minDepth));
	std::string fdepth = toString(buf);

	log << head << "*---------------------------------------*" << endl;
	log << head << "|           ITERATIONS RESULTS          |" << endl;
	log << head << "*---------------------------------------*" << endl;
	log << head << "| MIN RMS VALUE: " << minRMS << endl;
	log << head << "|  MIN ER VALUE: " << minER << endl;
	log << head << "|  ZTR TO APPLY: " << fdepth << endl;
	log << head << "*---------------------------------------*" << endl;

	log.close();

	return fdepth;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Origin* Hypo71::locate(PickList& pickList, double initLat, double initLon,
                       double initDepth, Time & initTime) throw (Core::GeneralException) {
	return locate(pickList);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Origin* Hypo71::relocate(const Origin* origin) throw (Core::GeneralException) {

	//! sharing originID
	_currentOriginID = origin->publicID();

	//! sharing last known origin's lat & lon situation
	_oLastLatitude = origin->latitude().value();
	_oLastLongitude = origin->longitude().value();

	_lastWarning = "";

	if ( origin == NULL )
		return NULL;

	bool emptyProfile = false;

	if ( _currentProfile == NULL ) {
		emptyProfile = true;
		throw LocatorException(string("Please select a profile down the list !"));
	}

	PickList picks;
	for (size_t i = 0; i < origin->arrivalCount(); ++i) {

		double weight = 1.0;

		Pick *pick = getPick(origin->arrival(i));

		DataModel::SensorLocation *sloc = getSensorLocation(pick);

		//! Passing on it if station's sensor location isn't found
		//! Note: prevent this to happen by having fully implemented datalesses
		if ( !sloc ) {
			throw StationNotFoundException("Station '" + pick->waveformID().networkCode()
			        + "." + pick->waveformID().stationCode() + "' has not been found in database");
		}

		try {
			if ( origin->arrival(i)->weight() <= 0 )
				weight = 0.0;
		}
		catch ( ... ) {
		}

		if ( pick != NULL ) {
			picks.push_back(WeightedPick(pick, weight));
		}

	}

	Origin *org;
	try {
		org = locate(picks);
	}
	catch ( GeneralException &exc ) {

		if ( emptyProfile )
			_currentProfile = NULL;
		throw exc;
	}

	return org;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string Hypo71::lastMessage(MessageType type) const {

	if ( type == Warning )
		return _lastWarning;
	return "";
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Hypo71::updateProfile(const std::string& name) {

	_currentProfile = NULL;
	Profile *prof = NULL;
	for (Profiles::iterator it = _profiles.begin(); it != _profiles.end();
	        ++it) {
		if ( it->name != name )
			continue;
		prof = &(*it);
		break;
	}

	if ( prof == _currentProfile )
		return;
	_currentProfile = prof;
	_controlFile.clear();

	//!--------------------------------------
	//! unset all allowed parameters values !
	//!--------------------------------------
	/*
	 for (ParameterMap::iterator it = _parameters.begin(); it != _parameters.end(); ++it) {
	 SEISCOMP_DEBUG("[plugin] [hypo71] it->first: %s | it->second: %s", it->first.c_str(), it->second.c_str());
	 it->second = "";
	 }
	 */

	if ( _currentProfile ) {
		string controlFile;
		if ( !_currentProfile->controlFile.empty() )
			controlFile = _currentProfile->controlFile;
		else if ( !_controlFilePath.empty() )
			controlFile = _controlFilePath;

		if ( !controlFile.empty() ) {

			ConfigFile config(controlFile);
			std::string blank;

			// Reset list parameters
			std::string test01;
			std::string test02;
			std::string test03;
			std::string test04;
			std::string test05;
			std::string test06;
			std::string test07;
			std::string test08;
			std::string test09;
			std::string test10;
			std::string test11;
			std::string test12;
			std::string test13;
			std::string test15;
			std::string test20;
			config.readInto(test01, "TEST(01)", blank);
			Hypo71::setParameter("TEST(01)", test01);
			config.readInto(test02, "TEST(02)", blank);
			Hypo71::setParameter("TEST(02)", test02);
			config.readInto(test03, "TEST(03)", blank);
			Hypo71::setParameter("TEST(03)", test03);
			config.readInto(test04, "TEST(04)", blank);
			Hypo71::setParameter("TEST(04)", test04);
			config.readInto(test05, "TEST(05)", blank);
			Hypo71::setParameter("TEST(05)", test05);
			config.readInto(test06, "TEST(06)", blank);
			Hypo71::setParameter("TEST(06)", test06);
			config.readInto(test07, "TEST(07)", blank);
			Hypo71::setParameter("TEST(07)", test07);
			config.readInto(test08, "TEST(08)", blank);
			Hypo71::setParameter("TEST(08)", test08);
			config.readInto(test09, "TEST(09)", blank);
			Hypo71::setParameter("TEST(09)", test09);
			config.readInto(test10, "TEST(10)", blank);
			Hypo71::setParameter("TEST(10)", test10);
			config.readInto(test11, "TEST(11)", blank);
			Hypo71::setParameter("TEST(11)", test11);
			config.readInto(test12, "TEST(12)", blank);
			Hypo71::setParameter("TEST(12)", test12);
			config.readInto(test13, "TEST(13)", blank);
			Hypo71::setParameter("TEST(13)", test13);
			config.readInto(test15, "TEST(15)", blank);
			Hypo71::setParameter("TEST(15)", test15);
			config.readInto(test20, "TEST(20)", blank);
			Hypo71::setParameter("TEST(20)", test20);


			// Control card parameters
			std::string ztr;
			std::string xnear;
			std::string xfar;
			std::string pos;
			std::string kfm;
			std::string kaz;
			config.readInto(ztr, "ZTR", blank);
			Hypo71::setParameter("ZTR", ztr);
			config.readInto(xnear, "XNEAR", blank);
			Hypo71::setParameter("XNEAR", xnear);
			config.readInto(xfar, "XFAR", blank);
			Hypo71::setParameter("XFAR", xfar);
			config.readInto(pos, "POS", blank);
			Hypo71::setParameter("POS", pos);
			config.readInto(kfm, "KFM", blank);
			Hypo71::setParameter("KFM", kfm);
			config.readInto(kaz, "KAZ", blank);
			Hypo71::setParameter("KAZ", kaz);


			// Instruction card parameters
			std::string knst;
			std::string inst;
			config.readInto(knst, "KNST", blank);
			Hypo71::setParameter("KNST", knst);
			config.readInto(inst, "INST", blank);
			Hypo71::setParameter("INST", inst);
		}
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



