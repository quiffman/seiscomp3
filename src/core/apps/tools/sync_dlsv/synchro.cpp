/*=====================================================================
    Name:	synchro.C

    Purpose:  	synchronisation between stationdb and seiscompdb

    Usage:   	will be called from script monitor.sh, 
		that is sheduled from synchronistation

    Language:   C++, ANSI standard.

    Author:     Peter de Boer, KNMI, Netherlands

    Revision:	2007-11-16	0.1	initial

=====================================================================*/
#include <seiscomp3/client/inventory.h>
#include "synchro.h"
#include "dataless.h"

using namespace std;

namespace Seiscomp {
namespace Applications {


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Synchro::Synchro(int argc, char **argv) : Client::Application(argc, argv) {
	setLoggingToStdErr(true);
	setMessagingEnabled(true);
	setDatabaseEnabled(true, true);
	setLoadInventoryEnabled(true);
	setAutoApplyNotifierEnabled(false);
	setInterpretNotifierEnabled(false);
	setPrimaryMessagingGroup("INVENTORY");
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Synchro::~Synchro() {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Synchro::createCommandLineDescription() {
	Client::Application::createCommandLineDescription();

	commandline().addGroup("ArcLink");
	commandline().addOption("ArcLink", "dcid", "datacenter/archive ID", &_dcid, false);
	commandline().addGroup("Mode");
	commandline().addOption("Mode", "dump", "do not update the database and dump resulting XML to stdout");
	commandline().addOption("Mode", "offline", "do not connect to the messaging server and start with an empty inventory");
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Synchro::validateParameters() {
	if( commandline().hasOption("dcid") )
		_init[ARCHIVE] = _dcid;

	if( commandline().hasOption("offline") ) {
		setMessagingEnabled(false);
		setDatabaseEnabled(false, false);
		Client::Inventory::Instance()->setInventory(new DataModel::Inventory);
	}

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Synchro::initConfiguration() {
	if ( !Client::Application::initConfiguration() )
		return false;

	// force logging to stderr even if logging.file = 1
	setLoggingToStdErr(true);
	
	try { _init[ARCHIVE] = configGetString("datacenterID"); } catch (...) {}

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Synchro::run() {
	vector<string> args = commandline().unrecognizedOptions();
	if ( args.size() != 1 )
	{
	   cout << "Usage: sync_dlsv [options] file" << endl;
	   return false;
	}

	if(_init.find(ARCHIVE) == _init.end()) {
		cout << "Please specify datacenter/archive ID" << endl;
		return false;
	}

	_syncID = name() + "_" + Core::Time::GMT().iso();

	Dataless *dl = new Dataless(_init, commandline().hasOption("dump"));
	dl->SynchronizeDataless(args[0], connection(), this);

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Synchro::syncCallback() {
	return sync(_syncID.c_str());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


} // namespace Applications
} // namespace Seiscomp

