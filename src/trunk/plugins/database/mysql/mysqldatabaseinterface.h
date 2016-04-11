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


#ifndef __SEISOMP_SERVICES_DATABASE_MYSQL_INTERFACE_H__
#define __SEISOMP_SERVICES_DATABASE_MYSQL_INTERFACE_H__

#include <seiscomp3/io/database.h>
#ifdef WIN32
#include <mysql.h>
#else
#include <mysql/mysql.h>
#endif


namespace Seiscomp {
namespace Database {


class MySQLDatabase : public Seiscomp::IO::DatabaseInterface {
	DECLARE_SC_CLASS(MySQLDatabase);
	
	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		MySQLDatabase();
		~MySQLDatabase();


	// ------------------------------------------------------------------
	//  Public interface
	// ------------------------------------------------------------------
	public:
		bool connect(const char *con);
		void disconnect();
		
		bool isConnected() const;

		void start();
		void commit();
		void rollback();

		bool execute(const char* command);
		bool beginQuery(const char* query);
		void endQuery();

		unsigned long lastInsertId(const char*);

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
		bool ping() const;
		bool query(const char *c, const char *comp);
		bool reconnect();


	private:
		MYSQL* _handle;
		MYSQL_RES* _result;
		MYSQL_ROW _row;
		//std::string _lastQuery;
		mutable int _fieldCount;
		mutable unsigned long *_lengths;
};


}
}


#endif
