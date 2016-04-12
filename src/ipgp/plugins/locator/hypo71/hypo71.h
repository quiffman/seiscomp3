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


#ifndef __IPGP_HYPO71_PLUGIN_H__
#define __IPGP_HYPO71_PLUGIN_H__



#include <seiscomp3/core/plugin.h>
#include <seiscomp3/seismology/locatorinterface.h>
#include <seiscomp3/config/config.h>
#include <string>



namespace Seiscomp {
namespace Seismology {

class Hypo71 : public LocatorInterface {

	public:
		Hypo71();
		~Hypo71();

	public:
		/**
		 * Initializes the locator and reads at least the path of the default control file used for Hypo71.
		 * @param config
		 * @return
		 */
		virtual bool init(const Seiscomp::Config::Config& config);

		/**
		 * Returns supported parameters to be changed.
		 */
		virtual IDList parameters() const;

		/**
		 * Returns the value of a parameter.
		 * @param name parameter name
		 * @return parameter value
		 */
		virtual std::string parameter(const std::string& name) const;

		/**
		 * Sets the value of a parameter.
		 * @param name parameter name
		 * @param value new parameter value
		 * @return true
		 */
		virtual bool setParameter(const std::string& name,
		                          const std::string& value);

		/**
		 * Generic way to get stored memory profiles
		 * @return current stored profiles as a list
		 **/
		virtual IDList profiles() const;

		/**
		 * Specifies the earth model to be used\n
		 * e.g. "Tectonic"
		 * @param name the profile name
		 */
		virtual void setProfile(const std::string& name);

		/**
		 * Locator interface method to recover fixed depth and distance cutoff boolean values from Scolv interface
		 * @return true/false 0/1
		 **/
		virtual int capabilities() const;

		/**
		 * Performs event localization by using a picks list and Hypo71 locator binary
		 * @param pickList object containing the event arrivals and phases
		 * @return located event as origin object
		 **/
		virtual DataModel::Origin* locate(PickList& pickList) throw (Core::GeneralException);

		/**
		 * Surcharged method who indirectly performs event localization by calling child locate method
		 * @param pickList list of picks
		 * @param initLat event initial latitude
		 * @param initLon event initial longitude
		 * @param initDepth event initial depth
		 * @param initTime event initial time
		 * @return located origin object
		 **/
		virtual DataModel::Origin* locate(PickList& pickList, double initLat,
		                                  double initLon, double initDepth,
		                                  Core::Time& initTime) throw (Core::GeneralException);

		/**
		 * Origin re-locator
		 * @param origin origin object to relocate
		 * @return origin object
		 */
		virtual DataModel::Origin*
		relocate(const DataModel::Origin* origin) throw (Core::GeneralException);

		/**
		 * Fetches last error message value
		 * @param type Message type
		 * @return "" or message value (string)
		 **/
		virtual std::string lastMessage(MessageType) const;

	private:
		/**
		 * Updates scolv Hypo71 profile parameters by reading control file values
		 * @param name as parameter's name in _profileName
		 **/
		void updateProfile(const std::string &name);

		/**
		 * Determines the station situation using passed latitude or longitude argument
		 * @param d latitude or longitude value
		 * @param card 0 = latitude / 1 = longitude
		 * @return N / S / E / W
		 **/
		std::string geoSituation(double d, int card);

		/**
		 * Generates extra blank characters if needed
		 * @param toFormat string value to format
		 * @param nb characters to achieve
		 * @param pos 0 or 1 to add blank before or after passed string
		 * @return formated string
		 **/
		std::string formatString(std::string toFormat, unsigned int nb, int pos,
		                         std::string sender = "") throw (Core::GeneralException);

		/**
		 * Converts string parameter to double
		 * @param s string to convert
		 * @return double value of passed string
		 **/
		double toDouble(std::string s);

		/**
		 * Converts passed string number into int number
		 * @param str string to convert
		 * @return int value of passed string
		 **/
		int toInt(std::string str);

		/**
		 * Converts decimal lat/lon into sexagesimal values deg min.dec
		 * @param value lat/lon as decimal
		 * @return (string) sexagesimal value of decimal input
		 **/
		std::string DecimalToSexagesimalHypo71(double value);

		/**
		 * Selects best ZTR value for seismic localization
		 * @param pickList list of picks of a origin
		 * @return ZRT value as a string
		 **/
		std::string getZTR(PickList& pickList) throw (Core::GeneralException);

		/**
		 * Converts HYPO71 sexagesimal origin (73° 59.14’ W) in a decimal origin (-73.9874°).
		 * @param deg double degrees input value
		 * @param min double decimated minute value
		 * @param pol string polar value of sexagesimal input (E/W/N/S)
		 * @return (string) decimal value of sexagesimal input
		 **/
		std::string SexagesimalToDecimalHypo71(double deg, double min,
		                                       std::string pol);

		std::string getSituation(double value, int type);

		/**
		 * Retrieves time value from picklist object
		 * @param pickList the C++ object picklist to iterate
		 * @param name string value of the station to locate
		 * @param phase string value of the phase to locate
		 * @param rtype return type value (0/1/2/3 ctime/hour/minute/sec.dec)
		 * @return time value as a double, if it has been found
		 **/
		double getTimeValue(PickList& pickList, std::string name,
		                    std::string phase, unsigned int rtype);

		/**
		 * Tells if a string is of a certain type
		 * @param str string to identify
		 * @param type 0/1 int/double
		 * @return true or false
		 **/
		bool stringIsOfType(string str, int type);

		/**
		 * Explodes a string using separator as a cut point
		 * @param str string to explode
		 * @param separator cut point
		 * @param results vector of exploded string
		 */
		void stringExplode(std::string str, std::string separator,
		                   std::vector<string>* results);

		/**
		 * Removes white spaces from passed string
		 * @param str string to strip whites paces
		 * @return stripped string of passed string
		 **/
		std::string stripSpace(string& str);

		int getH71Weight(PickList& pickList, std::string name, std::string type,
		                 double max);

		//! junk stuff

		/**
		 * Converts coordinates lat/lon from decimals to sexagesimal.
		 * This function uses extrapolated degrees to decimals formula from USGS
		 * -> http://en.wikipedia.org/wiki/Decimal_degrees
		 *
		 * DD=D+(M/60)+(S/3600)
		 *
		 * DD: decimal degrees
		 * D = trunc(DD)
		 * M = trunc(abs(DD)*60)mod60
		 * S = (abs(DD)*3600)mod60
		 * @param value value to convert
		 * @param card value cardinality 0 = latitude | 1 = longitude
		 * @return (string) sexagesimal value of decimal input
		 **/
		std::string DecimalToSexagesimal(double value, int polarity);

		/**
		 * Converts sexagesimal origin (-73° 59’ 14.64") in a decimal origin (-73.9874°).
		 * @param deg double value ex.-73
		 * @param min double value ex.59
		 * @param sec double value ex.14.64
		 * @return (string) decimal value of sexagesimal input
		 **/
		std::string SexagesimalToDecimal(double deg, double min, double sec,
		                                 std::string polarity);

		std::string getMinDeg(double value);

	private:
		struct Profile {
				std::string name;
				std::string earthModelID;
				std::string methodID;
				std::string controlFile;
				std::string velocityModelFile;
		};

		/* default reset list */
		struct defaultResetList {
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
		};

		/* custom reset list */
		struct customResetList {
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
		};

		/* default control card */
		struct defaultControlCard {
				std::string ztr;
				std::string xnear;
				std::string xfar;
				std::string pos;
				std::string iq;
				std::string kms;
				std::string kfm;
				std::string ipun;
				std::string imag;
				std::string ir;
				std::string iprn;
				std::string ktest;
				std::string kaz;
				std::string ksort;
				std::string ksel;
		};

		/* custom control card */
		struct customControlCard {
				std::string ztr;
				std::string xnear;
				std::string xfar;
				std::string pos;
				std::string iq;
				std::string kms;
				std::string kfm;
				std::string ipun;
				std::string imag;
				std::string ir;
				std::string iprn;
				std::string ktest;
				std::string kaz;
				std::string ksort;
				std::string ksel;
				std::string lat1;
				std::string lat2;
				std::string lon1;
				std::string lon2;
		};

		/* default instruction card */
		struct defaultInstructionCard {
				std::string ipro;
				std::string knst;
				std::string inst;
				std::string zres;
		};

		/* custom instruction card */
		struct customInstructionCard {
				std::string ipro;
				std::string knst;
				std::string inst;
				std::string zres;
		};

		typedef std::map<std::string, std::string> ParameterMap;
		typedef std::vector<std::string> TextLines;
		typedef std::list<Profile> Profiles;

		static IDList _allowedParameters;
		std::string _publicIDPattern;
		std::string _outputPath;
		std::string _h71inputFile;
		std::string _h71outputFile;
		std::string _controlFilePath;
		std::string _lastWarning;
		std::string _hypo71BinaryFile;
		std::string _hypo71ScriptFile;
		std::string _logFile;
		TextLines _controlFile;
		IDList _profileNames;
		double _fixedDepthGridSpacing;
		double _defaultPickError;
		bool _allowMissingStations;
		bool _useLastOriginAsReference;
		ParameterMap _parameters;
		Profiles _profiles;
		Profile *_currentProfile;
		std::string _currentOriginID;

		double _oLastLatitude;
		double _oLastLongitude;

		/* crustal model variables */
		std::string velocityModel;
		std::string depthModel;
};

}
}

#endif

