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


#define SEISCOMP_COMPONENT POSTGRESQL
#include <seiscomp3/logging/log.h>
#include <seiscomp3/core/plugin.h>
#include "postgresqldatabaseinterface.h"


namespace Seiscomp {
namespace Database {


IMPLEMENT_SC_CLASS_DERIVED(PostgreSQLDatabase,
                           Seiscomp::IO::DatabaseInterface,
                           "postgresql_database_interface");

REGISTER_DB_INTERFACE(PostgreSQLDatabase, "postgresql");
ADD_SC_PLUGIN("PostgreSQL database driver", "GFZ Potsdam <seiscomp-devel@gfz-potsdam.de>", 0, 9, 0)

PostgreSQLDatabase::PostgreSQLDatabase()
 : _handle(NULL), _result(NULL) {
	_defaultColumnPrefix = "m_";
	_columnPrefix = _defaultColumnPrefix;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
PostgreSQLDatabase::~PostgreSQLDatabase() {
	disconnect();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool PostgreSQLDatabase::open() {
	std::stringstream ss;
	if ( _port )
		ss << _port;

	_handle = PQsetdbLogin(_host.c_str(), ss.str().c_str(),
	                       NULL,
	                       NULL,
	                       _database.c_str(),
	                       _user.c_str(),
	                       _password.c_str());

	/* Check to see that the backend connection was successfully made */
	if ( PQstatus(_handle) != CONNECTION_OK ) {
		SEISCOMP_ERROR("Connection to database '%s' with user %s failed.", PQdb(_handle), _user.c_str());
		SEISCOMP_ERROR("%s", PQerrorMessage(_handle));

		disconnect();
		return false;
	}

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void PostgreSQLDatabase::disconnect() {
	if ( _result ) {
		PQclear(_result);
		_result = NULL;
	}

	PQfinish(_handle);
	_handle = NULL;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool PostgreSQLDatabase::isConnected() const {
	if ( _handle == NULL ) return false;
	ConnStatusType stat = PQstatus(_handle);
	if ( stat == CONNECTION_OK ) return true;

	SEISCOMP_ERROR("connection bad (%d) -> reconnect", stat);
	PQreset(_handle);
	return PQstatus(_handle) == CONNECTION_OK;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void PostgreSQLDatabase::start() {
	execute("start transaction");
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void PostgreSQLDatabase::commit() {
	execute("commit");
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void PostgreSQLDatabase::rollback() {
	execute("rollback");
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool PostgreSQLDatabase::execute(const char* command) {
	if ( !isConnected() || command == NULL ) return false;

	endQuery();

	SEISCOMP_DEBUG("statement: %s", command);
	_result = PQexec(_handle, command);
	if ( _result == NULL ) {
		SEISCOMP_ERROR("execute(\"%s\"): %s", command, PQerrorMessage(_handle));
		return false;
	}

	ExecStatusType stat = PQresultStatus(_result);
	if ( stat != PGRES_TUPLES_OK && stat != PGRES_COMMAND_OK ) {
		SEISCOMP_ERROR("QUERY/COMMAND failed");
		SEISCOMP_ERROR("  %s", command);
		SEISCOMP_ERROR("  %s", PQerrorMessage(_handle));
		PQclear(_result);
		_result = NULL;
		return false;
	}

	_nRows = PQntuples(_result);
	_fieldCount = PQnfields(_result);

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool PostgreSQLDatabase::beginQuery(const char* query) {
	return execute(query);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void PostgreSQLDatabase::endQuery() {
	_row = -1;
	_nRows = -1;
	if ( _result ) {
		PQclear(_result);
		_result = NULL;
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
unsigned long PostgreSQLDatabase::lastInsertId(const char* table) {
	if ( !execute((std::string("select currval('") + table + "_seq')").c_str()) )
		return 0;

	char* value = PQgetvalue(_result, 0, 0);
	return value?atoi(value):0;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool PostgreSQLDatabase::fetchRow() {
	++_row;

	if ( _row < _nRows ) return true;

	_row = _nRows;
	return false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int PostgreSQLDatabase::findColumn(const char* name) {
	return PQfnumber(_result, name);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int PostgreSQLDatabase::getRowFieldCount() const {
	return PQnfields(_result);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
const void* PostgreSQLDatabase::getRowField(int index) {
	if ( PQgetisnull(_result, _row, index) )
		return NULL;

	return PQgetvalue(_result, _row, index);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
size_t PostgreSQLDatabase::getRowFieldSize(int index) {
	return PQgetlength(_result, _row, index);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
