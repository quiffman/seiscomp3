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

#include "config.h"

#include <seiscomp3/core/strings.h>
#include <seiscomp3/logging/log.h>

#include <algorithm>
#include <iostream>
#include <vector>
#include <cstring>
#include <climits>
#include <cstdlib>


#if WIN32

#include <direct.h>
#define PATH_MAX MAX_PATH

char *realpath(const char *relPath, char *absPath) {
	return _fullpath(absPath, relPath, PATH_MAX);
}

#endif


#include <seiscomp3/config/environment.h>

namespace {

typedef std::vector<std::string> StringList;

}

namespace Seiscomp {

// ----------------------------------------------------------------------
// config.ipp
// ----------------------------------------------------------------------
#include <seiscomp3/config/config.ipp>


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
namespace {

std::string CONF_NULL_OBJECT = "___CONFIG_NULL_OBJECT___";
std::string quotable = "\\\t\n\v\f\r ,${}";

std::string removeApostrophe(const std::string &str)
{
	std::string tmpString(str);
	std::string::size_type pos = tmpString.find("\"");
	if (pos != std::string::npos)
		tmpString.erase(pos, pos + 1);

	pos = tmpString.rfind("\"");
	if (pos != std::string::npos)
		tmpString.erase(pos);

	return tmpString;
}

std::string stripEscapes(const std::string &str) {
	std::string tmpString(str);
	size_t pos = tmpString.find('\\');
	while ( pos != std::string::npos ) {
		if ( pos < tmpString.size()-1 && tmpString[pos+1] == '\"' )
			tmpString.erase(tmpString.begin() + pos);
		pos = tmpString.find('\\', pos+1);
	}

	return tmpString;
}

std::string escape(const std::string &str) {
	std::string tmpString(str);
	size_t pos = tmpString.find('\"');
	while ( pos != std::string::npos ) {
		tmpString.insert(tmpString.begin() + pos, '\\');
		pos = tmpString.find('\"', pos+2);
	}

	return tmpString;
}

std::string quote(const std::string &str) {
	if ( str.empty() )
		return "\"\"";
	if ( str.find_first_of(quotable) != std::string::npos )
		return std::string("\"") + str + "\"";

	return str;
}


} // namespace

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
template <>
std::string Config::get<std::string>(const std::string& name) const
throw(ConfigException)
{
	const Symbol* symbol = _symbolTable->get(name);
	if (!symbol)
		throw ConfigOptionNotFoundException(name);

	return symbol->values[0];
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
template <>
bool Config::get<bool>(const std::string& name) const
throw(ConfigException)
{
	const Symbol* symbol = _symbolTable->get(name);
	if (!symbol)
		throw ConfigOptionNotFoundException(name);

	std::string tmpVal = symbol->values[0];
	if (Core::compareNoCase(tmpVal, "true") == 0)
		return true;
	else if (Core::compareNoCase(tmpVal, "false") == 0)
		return false;

	bool value;
	if (!Core::fromString(value, symbol->values[0]))
		throw ConfigTypeConversionException(symbol->values[0]);

	return value;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
template <>
std::vector<std::string> Config::getVec<std::string>(const std::string& name) const
throw(ConfigException)
{
	const Symbol* symbol = _symbolTable->get(name);
	if (!symbol)
		throw ConfigOptionNotFoundException(name);

	std::vector<std::string> tmpVec;
	for (size_t i = 0; i < symbol->values.size(); ++i)
		tmpVec.push_back(stripEscapes(symbol->values[i]));

	return tmpVec;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
template <>
std::vector<bool> Config::getVec<bool>(const std::string& name) const
throw(ConfigException)
{
	const Symbol* symbol = _symbolTable->get(name);
	if (!symbol)
		throw ConfigOptionNotFoundException(name);

	std::vector<bool> values;
	for (size_t i = 0; i < symbol->values.size(); ++i)
	{
		if (Core::compareNoCase(symbol->values[i], "true") == 0)
			values.push_back(true);
		else if (Core::compareNoCase(symbol->values[i], "false") == 0)
			values.push_back(false);
		else
		{
			bool value;
			if (!Core::fromString(value, symbol->values[i]))
				throw ConfigTypeConversionException(symbol->values[i]);
			values.push_back(value);
		}
	}
	return values;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Config::Config() : _stage(0), _resolveReferences(true), _symbolTable(NULL) {
	init();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Config::~Config() {
	if (_file.is_open())
		_file.close();

	if ( _symbolTable ) {
		_symbolTable->decrementObjectCount();
		if ( _symbolTable->objectCount() <= 0 ) {
			//SEISCOMP_DEBUG("Deleting SymbolTable");
			delete _symbolTable;
			_symbolTable = NULL;
		}
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Config::readConfig(const std::string& file, int stage, bool raw) {
	_stage = stage;
	_resolveReferences = !raw;

	if ( !_symbolTable ) init();
	_line = 0;
	_fileName.assign(file);

	_file.open(_fileName.c_str(), std::ios_base::in);
	if ( _file.rdstate() != std::ios_base::goodbit ) {
		SEISCOMP_ERROR("Could not read file %s", _fileName.c_str());
		return false;
	}

	_symbolTable->addToIncludedFiles(_fileName);

	bool result = parseFile();

	if ( _file.is_open() )
		_file.close();

	if ( _file.rdstate() != std::ios_base::goodbit )
		_file.clear();

	return result;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Config::readInternalConfig(const std::string& file,
                                SymbolTable* symbolTable, int stage, bool raw) {
	if ( _symbolTable ) {
		_symbolTable->decrementObjectCount();
		if ( _symbolTable->objectCount() <= 0 ) {
			//SEISCOMP_DEBUG("Deleting old symboltable in assignment");
			delete _symbolTable;
			_symbolTable = NULL;
		}
	}
	_symbolTable = symbolTable;
	_symbolTable->incrementObjectCount();

	return readConfig(file, stage, raw);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Config::writeConfig(const std::string& file, bool localOnly)
{
	SymbolTable::iterator it = _symbolTable->begin();
	int firstLine = true;

	_file.open(file.c_str(), std::ios_base::out | std::ios_base::trunc);
	if (_file.rdstate() != std::ios_base::goodbit)
	{
		SEISCOMP_ERROR("Could not open file %s", file.c_str());
		return false;
	}

	for ( ; it != _symbolTable->end(); ++it)
	{
		if ( localOnly ) {
			if ( !(*it)->uri.empty() && (*it)->uri != file ) {
				continue;
			}
		}

		if (!firstLine)
			_file << std::endl;
		else
			firstLine = false;

		if (!(*it)->comment.empty())
			_file << (*it)->comment << std::endl;
		_file << (*it)->name << " = ";
		if ( (*it)->values.empty() )
			_file << "\"\"";
		else {
			for (size_t i = 0; i < (*it)->values.size(); ++i)
			{
				if (i != 0)
					_file << ", ";
				_file << quote(escape((*it)->values[i]));
			}
		}
		_file << std::endl;
	}

	_file.close();

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Config::writeConfig(bool localOnly)
{
	return writeConfig(_fileName, localOnly);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string Config::visitedFilesToString() {
	std::stringstream ss;
	SymbolTable::IncludedFiles::iterator fileIt = _symbolTable->includesBegin();
	for ( ; fileIt != _symbolTable->includesEnd(); ++fileIt)
		ss << *fileIt << std::endl;
	return ss.str();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string Config::symbolsToString() {
	return _symbolTable->toString();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Config::parseFile() {
	std::string entry;
	std::string comment;
	std::string line;
	bool stringMode = false;

	while ( getline(_file, line) ) {
		++_line;
		Core::trim(line);
		if ( line.empty() ) continue;

		// Handle comments
		std::string::iterator current  = line.begin();
		std::string::iterator next     = line.begin();
		std::string::iterator previous = line.begin();
		for ( ; current != line.end(); ++current ) {
			if ( current + 1 != line.end() )
				next = current + 1;
			if ( current != line.begin() )
				previous = current - 1;

			if ( *current == '"' && *previous != '\\' ) {
				stringMode = !stringMode;
			}
			else if ( *current == '#' && !stringMode ) {
				if ( !comment.empty() )
					comment += '\n';
				std::copy(current, line.end(), std::back_inserter(comment));
				break;
			}
			entry.push_back(*current);
		}

		entry = Core::trim(entry);
		if ( entry.empty() ) continue;

		if ( *entry.rbegin() != '\\' ) {
			if ( stringMode ) {
				SEISCOMP_ERROR("Unterminated \" found at line %d: %s", _line, line.c_str());
				return false;
			}

			if ( !handleEntry(entry + '\n', comment) ) {
				return false;
			}
			entry.clear();
			comment.clear();
		}
		else {
			entry.erase(entry.size() - 1);
		}
	}

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Config::init() {
	if ( _symbolTable == NULL ) {
		//SEISCOMP_DEBUG("Creating SymbolTable");
		_symbolTable = new SymbolTable;
	}
	_symbolTable->incrementObjectCount();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Config::handleEntry(const std::string& entry, const std::string& comment) {
	//SEISCOMP_DEBUG("Scanning entry: %s", entry.c_str());
	std::vector<std::string> tokens = tokenize(entry);

	//SEISCOMP_DEBUG("Parsing entry");
	if ( tokens.size() < 2 ) {
		SEISCOMP_ERROR("[%s:%d] Config entry malformed: To few parameter for %s",
		               _fileName.c_str(), _line, entry.c_str());
		return false;
	}

	if ( tokens[0][0] == '$' ) {
		SEISCOMP_ERROR("[%s:%d] Cannot assign to rvalue: %s",
		               _fileName.c_str(), _line, tokens[0].c_str());
		return false;
	}

	// Handle operators
	std::vector<std::string> parsedValues;
	if ( tokens[0] == "include" ) {
		if ( tokens.size() > 2 ) {
			SEISCOMP_ERROR("[%s:%d] Operator %s has to many operands -> %s file",
			               _fileName.c_str(), _line, tokens[0].c_str(), tokens[0].c_str());
			return false;
		}
		if ( !parseRValue(tokens[1], parsedValues) )
			return false;
		if ( !handleInclude(parsedValues[0]) ) {
			SEISCOMP_ERROR(
					"[%s:%d] Could not read include file %s",
					_fileName.c_str(),
					_line,
					parsedValues[0].c_str()
			);
			return false;
		}
	}
	else if ( tokens[0] == "del" ) {
		if ( tokens.size() == 1 ) {
			SEISCOMP_ERROR("[%s:%d] Missing operands for %s operator: %s",
					_fileName.c_str(), _line, tokens[0].c_str(), entry.c_str());
			return false;
		}

		std::string tmp;
		for ( size_t i = 1; i < tokens.size(); ++i )
			tmp += tokens[i];

		parsedValues.clear();
		if ( !parseRValue(tmp, parsedValues) )
			return false;

		std::vector<std::string>::const_iterator it = parsedValues.begin();
		for ( ; it != parsedValues.end(); ++it ) {
			if ( !_symbolTable->remove(*it) ) {
				SEISCOMP_ERROR("[%s:%d] Could not remove variable %s from symboltable",
						_fileName.c_str(), _line, (*it).c_str());
				return false;
			}
		}
	}
	else if ( tokens[1] == "=" ) {
		if ( tokens.size() < 3 ) {
			SEISCOMP_ERROR("[%s:%d] RValue missing in assignment: %s",
					_fileName.c_str(), _line, entry.c_str());
			return false;
		}

		std::vector<std::string> values;
		std::string tmp;
		for ( size_t i = 2; i < tokens.size(); ++i ) {
			tmp += tokens[i];
		}
		parsedValues.clear();
		if ( !parseRValue(tmp, parsedValues) )
			return false;
		std::vector<std::string>::const_iterator parsedValuesIt = parsedValues.begin();
		for ( ; parsedValuesIt != parsedValues.end(); ++parsedValuesIt ) {
			values.push_back(stripEscapes(*parsedValuesIt));
		}

		handleAssignment(tokens[0], tmp, values, comment);
	}
	else {
		SEISCOMP_ERROR("[%s:%d] Invalid entry: %s", _fileName.c_str(), _line, entry.c_str());
		return false;
	}

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Config::handleInclude(const std::string& fileName)
{
	if ( fileName.empty() ) return false;

	std::string tmpFileName(fileName);
	// Resolve ~ for home directory
	if (tmpFileName[0] == '~') {
		tmpFileName = Environment::Instance()->homeDir() + tmpFileName.substr(1);
	}

	bool isRelativePath = false;
	char oldPath[PATH_MAX];
	if ( tmpFileName[0] != '/' ) {
		isRelativePath = true;
		// Change to the config file path to be able to handle relative paths
		if ( getcwd(oldPath, PATH_MAX) ) {
			std::string::size_type pos = _fileName.rfind("/");
			if ( pos != std::string::npos )
				chdir(_fileName.substr(0, pos).c_str());
		}
	}


	tmpFileName = Environment::Instance()->absolutePath(tmpFileName);

	if ( !_symbolTable->hasFileBeenIncluded(tmpFileName) ) {
		//SEISCOMP_DEBUG("Handling include: %s", tmpFileName.c_str());
		Config conf;
		if ( !conf.readInternalConfig(tmpFileName, _symbolTable, _stage, !_resolveReferences) )
			return false;

	}

	if ( isRelativePath )
		chdir(oldPath);

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Config::handleAssignment(const std::string& name,
                              const std::string& content,
                              std::vector<std::string>& values,
                              const std::string& comment)
{
	_symbolTable->add(name, content, values, _fileName, comment, _stage);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Config::reference(const std::string& name, std::vector<std::string>& values)
{
	if ( _symbolTable ) {
		const Symbol* symbol = NULL;
		try {
			symbol = _symbolTable->get(name);
		}
		catch ( ConfigException& e ) {
			//SEISCOMP_DEBUG("%s", e.what());
		}

		if ( symbol ) {
			values = symbol->values;
			return true;
		}
		else {
			char *env = getenv(name.c_str());
			if ( env != NULL ) {
				values.clear();
				values.push_back(std::string(env));
				return true;
			}
		}
	}
	return false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::vector<std::string> Config::tokenize(const std::string& entry)
{
	std::vector<std::string> tokens;
	std::string nextToken;
	bool stringMode = false;
	std::string::const_iterator previousIt = entry.begin();

	std::string::const_iterator it = entry.begin();
	for ( ; it != entry.end(); ++it ) {
		if (it != entry.begin())
			previousIt = it - 1;

		bool isOperator = *it == '=' ||
		                  *it == ',';

		if ( stringMode ) {
			if ( *it == '"' && *previousIt != '\\' ) {
				stringMode = !stringMode;
				nextToken.push_back(*it);
				tokens.push_back(nextToken);
				nextToken.clear();
			}
			else {
				nextToken.push_back(*it);
			}
		}
		else if ( Core::isWhitespace(*it) ) {
			if ( !nextToken.empty() ) {
				tokens.push_back(nextToken);
				nextToken.clear();
			}
		}
		else if ( isOperator ) {
			if ( !nextToken.empty() ) {
				tokens.push_back(nextToken);
				nextToken.clear();
			}
			nextToken.push_back(*it);
			tokens.push_back(nextToken);
			nextToken.clear();
		}
		else if ( *it == '"' ) {
			stringMode = !stringMode;
			nextToken.push_back(*it);
		}
		else {
			nextToken.push_back(*it);
		}
	}

	return tokens;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Config::parseRValue(const std::string& entry,
                         std::vector<std::string>& parsedValues) {
	bool openCurlyBrace = false;
	bool stringMode     = false;
	std::string parsedEntry;
	std::vector<std::string> tokens;
	std::string::const_iterator it       = entry.begin();
	std::string::const_iterator previous = entry.begin();
	std::string::const_iterator next     = entry.begin();

	//SEISCOMP_DEBUG("entry: %s", entry.c_str());

	for ( ; it != entry.end(); ++it ) {
		if ( it != entry.begin() ) previous = it - 1;
		next = it + 1;

		if ( *it == '"' && *previous != '\\' ) {
			stringMode = !stringMode;
			if ( next != entry.end() && *next == '"' ) tokens.push_back("");
		}
		else if ( *it == '$' && !stringMode && _resolveReferences ) {
			std::string variable;
			if ( ++it == entry.end() ) {
				SEISCOMP_ERROR("[%s:%d] Standalone reference operator", _fileName.c_str(), _line);
				return false;
			}

			for ( ; it != entry.end(); ++it ) {
				if ( *it == '{' ) {
					openCurlyBrace = !openCurlyBrace;
				}
				else if ( *it == '}' ) {
					openCurlyBrace = !openCurlyBrace;
					break;
				}
				else {
					variable.push_back(*it);
				}
			}

			if ( openCurlyBrace ) {
				SEISCOMP_ERROR("[%s:%d] Missing brace in reference", _fileName.c_str(), _line);
				return false;
			}

			if ( !parsedEntry.empty() ) {
				tokens.push_back(parsedEntry);
				parsedEntry.clear();
			}

			std::vector<std::string> values;
			if ( !reference(variable, values) ) {
				SEISCOMP_DEBUG(
					"[%s:%d] Cannot reference variable: %s Assigning NULL object.",
					_fileName.c_str(),
					_line,
					variable.c_str()
				);
				values.push_back(CONF_NULL_OBJECT);
			}

			std::vector<std::string>::iterator valueIt = values.begin();
			for ( ; valueIt != values.end(); ++valueIt ) {
				if ( valueIt != values.begin() )
					tokens.push_back(",");
				tokens.push_back(*valueIt);
			}

			if ( it == entry.end() ) break;
		}
		else {
			if ( *it == ',' && !stringMode ) {
				if ( !parsedEntry.empty() ) {
					tokens.push_back(parsedEntry);
					parsedEntry.clear();
				}
				tokens.push_back(",");
			}
			else {
				if ( !Core::isWhitespace(*it) || stringMode )
					parsedEntry.push_back(*it);
			}
		}
	}

	if ( !parsedEntry.empty() )
		tokens.push_back(parsedEntry);

	//for ( size_t i = 0; i < tokens.size(); ++i )
	//	SEISCOMP_DEBUG("token%lu: %s", i ,tokens[i].c_str());

	// Apply grammar
	std::vector<std::string>::const_iterator currentIt  = tokens.begin();
	std::vector<std::string>::const_iterator previousIt = tokens.begin();
	std::vector<std::string>::const_iterator nextIt     = tokens.begin();
	std::string token;
	for ( ; currentIt != tokens.end(); ++currentIt ) {
		if ( currentIt != tokens.begin() ) previousIt = currentIt - 1;
		nextIt = currentIt + 1;

		if ( *currentIt == CONF_NULL_OBJECT ) {
			if ( nextIt != tokens.end() && *previousIt == "," && *nextIt == "," ) { ; }
			else if ( nextIt != tokens.end() && currentIt == tokens.begin() && *nextIt == "," ) { ; }
			else if ( nextIt == tokens.end() && *previousIt == "," ) { ; }
			else {
				SEISCOMP_ERROR(
					"[%s:%d] NULL object found in string concatenation: %s",
					_fileName.c_str(),
					_line,
					entry.c_str()
				);
				return false;
			}
		}
		else if ( *currentIt == "," ) {
			bool invalidList = *previousIt == "," ||
			                   (nextIt != tokens.end() && *nextIt == ",") ||
			                   nextIt == tokens.end();

			if ( invalidList ) {
				SEISCOMP_ERROR(
					"[%s:%d] Invalid list found: %s",
					_fileName.c_str(),
					_line,
					entry.c_str()
				);
				return false;
			}

			if ( *previousIt != CONF_NULL_OBJECT )
				parsedValues.push_back(token);
			token.clear();

			continue;
		}
		else {
			token += *currentIt;
			if ( nextIt == tokens.end() ) {
				parsedValues.push_back(token);
			}
		}
	}

	//for ( size_t i = 0; i < parsedValues.size(); ++i )
	//	SEISCOMP_DEBUG("value%lu: %s", i, parsedValues[i].c_str());

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
int Config::getInt(const std::string& name) const
throw(ConfigException)
{
	return get<int>(name);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int Config::getInt(const std::string& name, bool* error) const
{
	return get<int>(name, error);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Config::getInt(int& value, const std::string& name) const
{
	return get<int>(value, name);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
double Config::getDouble(const std::string& name) const
throw(ConfigException)
{
	return get<double>(name);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
double Config::getDouble(const std::string& name, bool* error) const
{
	return get<double>(name, error);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Config::getDouble(double& value, const std::string& name) const
{
	return get<double>(value, name);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
bool Config::getBool(const std::string& name) const
throw(ConfigException)
{
	return get<bool>(name);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Config::getBool(const std::string& name, bool* error) const
{
	return get<bool>(name, error);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Config::getBool(bool& value, const std::string& name) const
{
	return get<bool>(value, name);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string Config::getPath(const std::string& name) const
throw(ConfigException) {
	bool error = false;
	std::string value = get<std::string>(name, &error);
	if ( error == true ) throw ConfigOptionNotFoundException(name);

	return Environment::Instance()->absolutePath(value);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string Config::getPath(const std::string& name, bool* error) const {
	*error = false;

	std::string value = get<std::string>(name, error);
	if ( *error == true ) return "";

	return Environment::Instance()->absolutePath(value);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Config::getPath(std::string& value, const std::string& name) const {
	if ( !get<std::string>(value, name) ) {
		value == "";
		return false;
	}

	value = Environment::Instance()->absolutePath(value);
	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string Config::getString(const std::string& name) const
throw(ConfigException)
{
	return get<std::string>(name);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string Config::getString(const std::string& name, bool* error) const
{
	return get<std::string>(name, error);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Config::getString(std::string& value, const std::string& name) const
{
	return get<std::string>(value, name);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Config* Config::Instance(const std::string& fileName)
{
	static Config conf;
	if ( !conf.readConfig(fileName) )
		return NULL;
	return &conf;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
std::vector<int> Config::getInts(const std::string& name) const
throw(ConfigException)
{
	return getVec<int>(name);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::vector<int> Config::getInts(const std::string& name, bool* error) const
{
	return getVec<int>(name, error);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
std::vector<double> Config::getDoubles(const std::string& name) const
throw(ConfigException)
{
	return getVec<double>(name);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::vector<double> Config::getDoubles(const std::string& name, bool* error) const
{
	return getVec<double>(name, error);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
std::vector<bool> Config::getBools(const std::string& name) const
throw(ConfigException)
{
	return getVec<bool>(name);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::vector<bool> Config::getBools(const std::string& name, bool* error) const
{
	return getVec<bool>(name, error);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
std::vector<std::string> Config::getPaths(const std::string& name) const
throw(ConfigException)
{
	StringList values = getVec<std::string>(name);
	for ( StringList::iterator it = values.begin();
		  it != values.end(); ++it ) {
		*it =  Environment::Instance()->absolutePath(*it);
	}
	return values;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::vector<std::string> Config::getPaths(const std::string& name, bool* error) const
{
	*error = false;
	StringList values = getVec<std::string>(name, error);
	if ( *error ) return values;

	for ( StringList::iterator it = values.begin();
		  it != values.end(); ++it ) {
		*it =  Environment::Instance()->absolutePath(*it);
	}

	return values;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
std::vector<std::string> Config::getStrings(const std::string& name) const
throw(ConfigException)
{
	return getVec<std::string>(name);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::vector<std::string> Config::getStrings(const std::string& name, bool* error) const
{
	return getVec<std::string>(name, error);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Config::setInt(const std::string& name, int value)
{
	return set<int>(name, value);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Config::setDouble(const std::string& name, double value)
{
	return set<double>(name, value);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Config::setBool(const std::string& name, bool value)
{
	return set<bool>(name, value);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Config::setPath(const std::string& name, const std::string& value)
{
	return set<std::string>(name, value);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Config::setString(const std::string& name, const std::string& value)
{
	return set<std::string>(name, value);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Config::setInts(const std::string& name, const std::vector<int>& values)
{
	return set<int>(name, values);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Config::setDoubles(const std::string& name, const std::vector<double>& values)
{
	return set<double>(name, values);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Config::setBools(const std::string& name, const std::vector<bool>& values)
{
	return set<bool>(name, values);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Config::setPaths(const std::string& name, const std::vector<std::string>& values)
{
	return set<std::string>(name, values);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Config::setStrings(const std::string& name, const std::vector<std::string>& values)
{
	return set<std::string>(name, values);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
SymbolTable *Config::symbolTable() const {
	return _symbolTable;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
bool Config::remove(const std::string& name)
{
	return _symbolTable->remove(name);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



} // namespace Seiscomp
