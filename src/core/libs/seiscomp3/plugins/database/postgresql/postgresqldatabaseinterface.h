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


#ifndef __SEISOMP_SERVICES_DATABASE_POSTGRESQL_INTERFACE_H__
#define __SEISOMP_SERVICES_DATABASE_POSTGRESQL_INTERFACE_H__

#include <seiscomp3/io/database.h>
#include <libpq-fe.h>


namespace Seiscomp {
namespace Database {


class PostgreSQLDatabase : public Seiscomp::IO::DatabaseInterface {
	DECLARE_SC_CLASS(PostgreSQLDatabase);
	
	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		PostgreSQLDatabase();
		~PostgreSQLDatabase();


	// ------------------------------------------------------------------
	//  Public interface
	// ------------------------------------------------------------------
	public:
		void disconnect();
		
		bool isConnected() const;

		void start();
		void commit();
		void rollback();

		bool execute(const char* command);
		bool beginQuery(const char* query);
		void endQuery();

		unsigned long lastInsertId(const char* table);

		bool fetchRow();
		int findColumn(const char* name);
		int getRowFieldCount() const;
		const void* getRowField(int index);
		size_t getRowFieldSize(int index);


	// ------------------------------------------------------------------
	//  Protected interface
	// ------------------------------------------------------------------
	protected:
		bool open();


	// ------------------------------------------------------------------
	//  Implementation
	// ------------------------------------------------------------------
	private:
		PGconn *_handle;
		PGresult *_result;
		int _row;
		int _nRows;
		int _fieldCount;
};


}
}


#endif
