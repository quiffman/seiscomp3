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

#ifndef __SEISCOMP_CONFIG_SYMBOLTABLE__
#define __SEISCOMP_CONFIG_SYMBOLTABLE__

#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>

#include <seiscomp3/config/api.h>


namespace Seiscomp {

struct SC_CORE_CONFIG_API Symbol {

	typedef std::vector<std::string> Values;

	Symbol(const std::string& name,
	       const std::vector<std::string>& values,
	       const std::string& uri,
	       const std::string& comment,
	       int stage = -1);
	Symbol();

	void set(const std::string& name,
	         const std::vector<std::string>& values,
	         const std::string& uri,
	         const std::string& comment,
	         int stage = -1);

	bool operator ==(const Symbol& symbol) const;

	std::string toString() const;

	std::string name;
	std::string content;
	Values      values;
	std::string uri;
	std::string comment;
	int         stage;
};



class SC_CORE_CONFIG_API SymbolTable {

	private:
		typedef std::map<std::string, Symbol> Symbols;
		typedef std::vector<Symbol*> SymbolOrder;

	public:
		typedef SymbolOrder::const_iterator iterator;
		typedef std::set<std::string> IncludedFiles;
		typedef IncludedFiles::iterator file_iterator;

	public:
		SymbolTable();


	public:
		void add(const std::string& name,
		         const std::string& content,
		         const std::vector<std::string>& values,
		         const std::string& uri,
		         const std::string& comment = "",
		         int stage=-1);

		void add(const Symbol& symbol);

		Symbol* get(const std::string& name);
		const Symbol* get(const std::string& name) const;

		bool remove(const std::string& name);

		int incrementObjectCount();
		int decrementObjectCount();
		int objectCount() const;

		std::string toString() const;

		bool hasFileBeenIncluded(const std::string& fileName);
		void addToIncludedFiles(const std::string& fileName);

		file_iterator includesBegin();
		file_iterator includesEnd();

		iterator begin();
		iterator end();


	private:
		Symbols       _symbols;
		SymbolOrder   _symbolOrder;
		IncludedFiles _includedFiles;
		int           _objectCount;
};

} // namespace Seiscomp

#endif
