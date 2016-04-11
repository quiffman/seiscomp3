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


#ifndef __SEISCOMP_CONFIG_H__
#define __SEISCOMP_CONFIG_H__

#include <seiscomp3/config/exceptions.h>
#include <seiscomp3/config/symboltable.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <map>


namespace Seiscomp {


/**
 * This is a class for reading and writing configuration files. Currently the
 * following datatypes are supported: bool, int, double and std::string as well as
 * lists of the datatypes
 */
class SC_CORE_CONFIG_API Config {

	// ------------------------------------------------------------------------
	// X'struction
	// ------------------------------------------------------------------------
	public:
		Config();
		~Config();


	// ------------------------------------------------------------------------
	// Public interface
	// ------------------------------------------------------------------------
	public:

		/** Reads the given configuration file.
		 * @param file name of the configuration files
		 * @param stage Optional stage value to be set to each read symbol
		 * @param raw Raw mode which does not resolv references like ${var}
		 * @return true on success
		 */
		bool readConfig(const std::string& file, int stage=-1, bool raw=false);

		/** Writes the configuration to the given configuration file.
		 * @param file name of the configuarion files
		 * @param localOnly write only value read from this file and
		 *                  new entries
		 * @return true on success
		 */
		bool writeConfig(const std::string& file, bool localOny = true);

		/** Writes the configuration to the file which was given to
		 * readConfing
		 * @return true on success
		 */
		bool writeConfig(bool localOnly = true);

		/** Returns the symboltabel as string */
		std::string symbolsToString();

		/** Returns the names of the visited files */
		std::string visitedFilesToString();

		//! Gets an integer from the configuration file
		//! @param name name of the element
		//! @return value
		int getInt(const std::string& name) const throw(ConfigException);
		int getInt(const std::string& name, bool* error) const;
		bool getInt(int& value, const std::string& name) const;

		bool setInt(const std::string& name, int value);

		/** Gets a double from the configuration file
		 * @param name name of the element
		 * @return double
		 */
		double getDouble(const std::string& name) const throw(ConfigException);
		double getDouble(const std::string& name, bool* error) const;
		bool getDouble(double& value, const std::string& name) const;

		bool setDouble(const std::string& name, double value);

		/** Gets an boolean from the configuration file
		 * @param name name of the element
		 * @return boolean
		 */
		bool getBool(const std::string& name) const throw(ConfigException);
		bool getBool(const std::string& name, bool* error) const;
		bool getBool(bool& value, const std::string& name) const;

		bool setBool(const std::string& name, bool value);

		/** Gets a path from the configuration file
		 * @param name name of the element
		 * @return string
		 */
		std::string getPath(const std::string& name) const throw(ConfigException);
		std::string getPath(const std::string& name, bool* error) const;
		bool getPath(std::string&, const std::string& name) const;

		bool setPath(const std::string& name, const std::string& value);

		/** Gets a string from the configuration file
		 * @param name name of the element
		 * @return string
		 */
		std::string getString(const std::string& name) const throw(ConfigException);
		std::string getString(const std::string& name, bool* error) const;
		bool getString(std::string& value, const std::string& name) const;

		bool setString(const std::string& name, const std::string& value);

		/** Removes the symbol with the given name from the symboltable.
		 * @param name Symbol to be removed
		 */
		bool remove(const std::string& name);

		std::vector<int> getInts(const std::string& name) const
		throw(ConfigException);

		std::vector<int> getInts(const std::string& name, bool* error) const;

		bool setInts(const std::string& name, const std::vector<int>& values);

		std::vector<double> getDoubles(const std::string& name) const
		throw(ConfigException);

		std::vector<double> getDoubles(const std::string& name, bool* error) const;

		bool setDoubles(const std::string& name, const std::vector<double>& values);

		std::vector<bool> getBools(const std::string& name) const
		throw(ConfigException);

		std::vector<bool> getBools(const std::string& name, bool* error) const;

		bool setBools(const std::string& name, const std::vector<bool>& values);

		std::vector<std::string> getPaths(const std::string& name) const
		throw(ConfigException);
		std::vector<std::string> getPaths(const std::string& name, bool* error) const;

		bool setPaths(const std::string& name, const std::vector<std::string>& values);

		std::vector<std::string> getStrings(const std::string& name) const
		throw(ConfigException);

		std::vector<std::string> getStrings(const std::string& name, bool* error) const;

		bool setStrings(const std::string& name, const std::vector<std::string>& values);

		SymbolTable *symbolTable() const;

		/** Returns a ready to use config object for the given filename.
		 * The Instance must not be deleted!
		 * @return Config Instance or NULL if an error occured
		 */
		static Config* Instance(const std::string& fileName);


		// ----------------------------------------------------------------------
		// Protected interface
		// ----------------------------------------------------------------------
	protected:
		/** Parses the given file
		 * @return true on success false on failure
		 */
		bool parseFile(); // virtual candidate


		// ------------------------------------------------------------------------
		// Private interface
		// ------------------------------------------------------------------------
	private:
		void init();
		bool handleEntry(const std::string& entry, const std::string& comment);
		bool handleInclude(const std::string& fileName);
		void handleAssignment(const std::string& name, const std::string& content,
		                      std::vector<std::string>& values,
		                      const std::string& comment);
		bool reference(const std::string& name, std::vector<std::string>& value);
		std::vector<std::string> tokenize(const std::string& entry);
		bool parseRValue(const std::string& entry,
		                 std::vector<std::string>& parsedValues);

		bool readInternalConfig(const std::string& file, SymbolTable* symbolTable,
		                        int stage=-1, bool raw=false);

		template <typename T>
		T get(const std::string& name) const
		throw(ConfigException);

		template <typename T>
		T get(const std::string& name, bool* error) const;

		template <typename T>
		bool get(T& value, const std::string& name) const;

		template <typename T>
		std::vector<T> getVec(const std::string& name) const
		throw(ConfigException);

		template <typename T>
		std::vector<T> getVec(const std::string& name, bool* error) const;

		template <typename T>
		void add(const std::string& name, const T& value);

		template <typename T>
		void add(const std::string& name, const std::vector<T>& values);

		/** Sets an value in the configuration file
		 * @param element name of the element
		 * @param value value for the element */
		template <typename T>
		bool set(const std::string& name, const T& value);

		template <typename T>
		bool set(const std::string& name, const std::vector<T>& values);


		// ------------------------------------------------------------------------
		// Private data members
		// ------------------------------------------------------------------------
	private:
		int          _stage;
		int          _line;
		bool         _isReadable;
		bool         _resolveReferences;
		std::string  _fileName;
		std::fstream _file;

		SymbolTable*  _symbolTable;

};


} // namespace Seiscomp

#endif
