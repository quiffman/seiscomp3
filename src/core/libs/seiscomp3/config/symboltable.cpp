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

#define SEISCOMP_COMPONENT Conf
#include "symboltable.h"

#include <sstream>
#include <ctype.h>
#include <algorithm>

#include <seiscomp3/logging/log.h>


namespace Seiscomp {

namespace {

void makeUpper(std::string &dest, const std::string &src) {
	dest = src;
	for ( size_t i = 0; i < src.size(); ++i )
		dest[i] = toupper(src[i]);
}

std::string makeUpper(const std::string &src) {
	std::string dest;
	makeUpper(dest, src);
	return dest;
}

}


Symbol::Symbol(const std::string& name,
               const std::vector<std::string>& values,
               const std::string& uri,
               const std::string& comment,
               int s)
: name(name),
  values(values),
  uri(uri),
  comment(comment),
  stage(s){
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Symbol::Symbol() : stage(-1) {}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Symbol::set(const std::string& name,
                 const std::vector<std::string>& values,
                 const std::string& uri,
                 const std::string& comment,
                 int stage) {
	this->name    = name;
	this->values  = values;
	this->uri     = uri;
	this->comment = comment;
	this->stage   = stage;
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Symbol::operator ==(const Symbol& symbol) const {
	if (name == symbol.name)
		return true;
	return false;
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string Symbol::toString() const {
	std::stringstream ss;
	if (!comment.empty())
		ss << comment;
	ss << name << " = ";
	Values::const_iterator valueIt = values.begin();
	for ( ; valueIt != values.end(); ++valueIt ) {
		if ( valueIt != values.begin() )
			ss << ", ";
		ss << *valueIt;
	}
	ss << " in " << uri;
	return ss.str();
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SymbolTable::SymbolTable() : _objectCount(0) { }
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void SymbolTable::add(const std::string& name,
                      const std::string& content,
                      const std::vector<std::string>& values,
                      const std::string& uri,
                      const std::string& comment,
                      int stage) {
	Symbol* symbol = get(name);
	if ( !symbol ) {
		Symbol &newSymbol = _symbols[makeUpper(name)];
		newSymbol = Symbol(name, values, uri, comment, stage);
		newSymbol.content = content;
		_symbolOrder.push_back(&newSymbol);
	}
	else {
		symbol->set(name, values, uri, comment, stage);
		symbol->content = content;
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void SymbolTable::add(const Symbol& symbol)
{
	Symbol* sym = get(symbol.name);
	if ( !sym ) {
		Symbol &newSymbol = _symbols[makeUpper(symbol.name)];
		newSymbol = symbol;
		_symbolOrder.push_back(&newSymbol);
	}
	else
		*sym = symbol;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool SymbolTable::remove(const std::string& name)
{
	Symbols::iterator it = _symbols.find(makeUpper(name));
	if ( it != _symbols.end() ) {
		SymbolOrder::iterator ito = std::find(_symbolOrder.begin(), _symbolOrder.end(), &it->second);
		if ( ito != _symbolOrder.end() )
			_symbolOrder.erase(ito);
		_symbols.erase(it);
		return true;
	}

	return false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Symbol* SymbolTable::get(const std::string& name) {
	Symbols::iterator it = _symbols.find(makeUpper(name));
	if ( it != _symbols.end() )
		return &it->second;

	return NULL;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
const Symbol* SymbolTable::get(const std::string& name) const
{
	Symbols::const_iterator it = _symbols.find(makeUpper(name));
	if ( it != _symbols.end() )
		return &it->second;

	return NULL;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int SymbolTable::incrementObjectCount()
{
	return ++_objectCount;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int SymbolTable::decrementObjectCount()
{
	return --_objectCount;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int SymbolTable::objectCount() const
{
	return _objectCount;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string SymbolTable::toString() const
{
	std::stringstream ss;
	SymbolOrder::const_iterator symbolIt = _symbolOrder.begin();
	for ( ; symbolIt != _symbolOrder.end(); ++symbolIt)
		ss << (*symbolIt)->toString() << std::endl;

	return ss.str();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool SymbolTable::hasFileBeenIncluded(const std::string& fileName)
{
	IncludedFiles::iterator it = _includedFiles.find(fileName);
	if (it != _includedFiles.end())
		return true;
	return false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void SymbolTable::addToIncludedFiles(const std::string& fileName)
{
	_includedFiles.insert(fileName);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SymbolTable::file_iterator SymbolTable::includesBegin() {
	return _includedFiles.begin();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SymbolTable::file_iterator SymbolTable::includesEnd() {
	return _includedFiles.end();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SymbolTable::iterator SymbolTable::begin()
{
	return _symbolOrder.begin();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SymbolTable::iterator SymbolTable::end()
{
	return _symbolOrder.end();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




} // namespace Seiscomp
