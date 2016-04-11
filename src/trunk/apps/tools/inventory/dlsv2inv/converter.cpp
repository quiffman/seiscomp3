/*==============================================================================
    Name:       converter.cpp

    Purpose:    convertion between stationdb and seiscomp xml

    Language:   C++, ANSI standard.

    Author:     Peter de Boer, KNMI, Netherlands
                Adopted by Jan Becker, gempa GmbH

    Revision:   2012-05-02

==============================================================================*/
#include <seiscomp3/client/inventory.h>
#include <seiscomp3/io/archive/xmlarchive.h>
#include "converter.h"
#include "dataless.h"

using namespace std;

namespace Seiscomp {
namespace Applications {


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Converter::Converter(int argc, char **argv) : Client::Application(argc, argv) {
	setMessagingEnabled(false);
	setDatabaseEnabled(false, false);
	setLoggingToStdErr(true);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Converter::~Converter() {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Converter::createCommandLineDescription() {
	Client::Application::createCommandLineDescription();

	commandline().addGroup("ArcLink");
	commandline().addOption("ArcLink", "dcid", "datacenter/archive ID", &_dcid, false);

	commandline().addGroup("Convert");
	commandline().addOption("Convert", "formatted,f", "Enables formatted output");
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Converter::validateParameters() {
	if( commandline().hasOption("dcid") )
		_init[ARCHIVE] = _dcid;
	else
		_init[ARCHIVE] = "";

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Converter::initConfiguration() {
	if ( !Client::Application::initConfiguration() )
		return false;

	// force logging to stderr even if logging.file = 1
	setLoggingToStdErr(true);
	
	try { _init[ARCHIVE] = configGetString("datacenterID"); } catch (...) {}

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Converter::run() {
	vector<string> args = commandline().unrecognizedOptions();
	if ( args.size() < 1 ) {
		cerr << "Usage: dlsv2inv [options] input [output=stdout]" << endl;
		return false;
	}

	DataModel::InventoryPtr inv = new DataModel::Inventory;
	Dataless *dl = new Dataless(_init);
	if ( !dl->SynchronizeDataless(inv.get(), args[0]) ) {
		cerr << "Error processing data" << endl;
		return false;
	}

	IO::XMLArchive ar;

	if ( args.size() > 1 ) {
		if ( !ar.create(args[1].c_str()) ) {
			cerr << "Cannot create " << args[1] << endl;
			return false;
		}
	}
	else {
		if ( !ar.create("-") ) {
			cerr << "Cannot open stdout for output" << endl;
			return false;
		}
	}

	ar.setFormattedOutput(commandline().hasOption("formatted"));
	ar << inv;
	ar.close();

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
} // namespace Applications
} // namespace Seiscomp

