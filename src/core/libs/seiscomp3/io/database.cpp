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


#define SEISCOMP_COMPONENT Database
#include <seiscomp3/io/database.h>
#include <seiscomp3/core/strings.h>
#include <seiscomp3/core/interfacefactory.ipp>
#include <seiscomp3/logging/log.h>

#include <string.h>

IMPLEMENT_INTERFACE_FACTORY(Seiscomp::IO::DatabaseInterface, SC_CORE_IO_API);

namespace Seiscomp {
namespace IO {


using namespace std;


IMPLEMENT_SC_ABSTRACT_CLASS(DatabaseInterface, "DatabaseInterface");


DatabaseInterface::DatabaseInterface() {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
DatabaseInterface::~DatabaseInterface() {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
DatabaseInterface* DatabaseInterface::Create(const char* service) {
	if ( service == NULL ) return NULL;
	
	return DatabaseInterfaceFactory::Create(service);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
DatabaseInterface* DatabaseInterface::Open(const char* uri) {
	const char* tmp;
	std::string service;
	std::string source;
	std::string type;

	tmp = strstr(uri, "://");
	if ( tmp ) {
		std::copy(uri, tmp, std::back_inserter(service));
		uri = tmp + 3;
	}
	
	source = uri;

	SEISCOMP_DEBUG("trying to open database %s://%s", service.c_str(), source.c_str());

	DatabaseInterface* db = Create(service.c_str());
	if ( !db ) {
		SEISCOMP_ERROR("Database driver '%s' is not supported", service.c_str());
		return NULL;
	}

	if ( !db->connect(source.c_str()) ) {
		SEISCOMP_ERROR("Connection to failed to %s", source.c_str());
		delete db;
		return NULL;
	}

	return db;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool DatabaseInterface::connect(const char* con) {
	if ( isConnected() )
		return false;

	_host = "";
	_user = "";
	_password = "";
	_database = "";
	_port = 0;
	_columnPrefix = _defaultColumnPrefix;

	string connection = con;
	string params;
	size_t pos = connection.find('?');
	if ( pos != string::npos ) {
		params = connection.substr(pos+1);
		connection.erase(connection.begin() + pos, connection.end());
	}

	vector<string> tokens;
	if ( Core::split(tokens, connection.c_str(), "@") >= 2 ) {
		string login = tokens[0];
		_database = tokens[1];
	
		Seiscomp::Core::split(tokens, login.c_str(), ":");
		_user = tokens.size() > 0?tokens[0]:"";
		_password = tokens.size() > 1?tokens[1]:"";
	}
	else
		_database = tokens[0];

	size_t splitter = _database.find_first_of('/');
	if ( splitter != string::npos ) {
		tokens.resize(2);
		tokens[0] = _database.substr(0, splitter);
		tokens[1] = _database.substr(splitter+1);
	}
	else
		tokens[0] = _database;

	_host = tokens.size() < 2?"":tokens[0];
	_database = tokens.size() < 2?tokens[0]:tokens[1];

	Core::split(tokens, _host.c_str(), ":");
	_host = tokens[0].empty()?"localhost":tokens[0];
	if ( tokens.size() > 1 )
		Seiscomp::Core::fromString(_port, tokens[1]);

	Core::split(tokens, params.c_str(), "&");
	if ( !tokens.empty() ) {
		for ( size_t i = 0; i < tokens.size(); ++i ) {
			vector<string> param;
			Core::split(param, tokens[i].c_str(), "=");
			if ( !params.empty() ) {
				if ( param[0] == "column_prefix" ) {
					if ( param.size() >= 2 )
						_columnPrefix = param[1];
				}
			}
		}
	}

	return open();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
const char* DatabaseInterface::defaultValue() const {
	return "default";
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string DatabaseInterface::timeToString(const Seiscomp::Core::Time& t) {
	return t.toString("%Y-%m-%d %H:%M:%S");
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Seiscomp::Core::Time DatabaseInterface::stringToTime(const char* str) {
	Seiscomp::Core::Time t;
	if ( str == NULL ) return t;
	t.fromString(str, "%F %T");
	return t;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
const std::string& DatabaseInterface::columnPrefix() const {
	return _columnPrefix;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string DatabaseInterface::convertColumnName(const std::string& name) const {
	return _columnPrefix + name;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string DatabaseInterface::getRowFieldString(int index) {
	const void *data = getRowField(index);
	if ( !data ) return "";
	return (const char*)data;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
