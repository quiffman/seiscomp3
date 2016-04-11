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





#ifndef __APPS_PICKER_STATIONCONFIG_H__
#define __APPS_PICKER_STATIONCONFIG_H__

#include <seiscomp3/datamodel/configmodule.h>
#include <seiscomp3/processing/parameters.h>

#include <string>
#include <map>


namespace Seiscomp {

class Config;

namespace Applications {
namespace Picker {


struct StreamConfig {
	StreamConfig();
	StreamConfig(double on, double off, double tcorr,
	             const std::string &f);

	std::string  locCode;
	std::string  channel;

	OPT(double)  triggerOn;
	OPT(double)  triggerOff;
	OPT(double)  timeCorrection;

	std::string  filter;

	bool         updatable;

	Processing::ParametersPtr parameters;
};


class StationConfig {
	public:
		typedef std::pair<std::string, std::string> Key;
		typedef std::map<Key, StreamConfig> ConfigMap;
		typedef ConfigMap::const_iterator const_iterator;


	public:
		StationConfig();

		void setDefault(const StreamConfig &entry);

		void read(const Seiscomp::Config *config, const DataModel::ConfigModule *module);
		bool read(const std::string &staConfFile);

		const StreamConfig *get(const std::string &net, const std::string &sta) const;

		const_iterator begin() const;
		const_iterator end() const;

		void dump() const;


	private:
		const StreamConfig *getBestWildcard(const std::string &net, const std::string &sta) const;


	private:
		StreamConfig _default;
		ConfigMap _stationConfigs;
};


}
}
}


#endif
