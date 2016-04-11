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

#define SEISCOMP_COMPONENT sync_dlsv
#include <seiscomp3/logging/log.h>

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

	_net_start_str = "1980-01-01"; // default
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
	commandline().addOption("ArcLink", "dcid", "datacenter/archive ID", &_dcid);
	commandline().addOption("ArcLink", "net-description", "override network description", &_net_description);
	commandline().addOption("ArcLink", "net-start", "set network start time", &_net_start_str);
	commandline().addOption("ArcLink", "net-end", "set network end time", &_net_end_str);
	commandline().addOption("ArcLink", "net-type", "set network type (VBB, SM, etc.)", &_net_type);
	commandline().addOption("ArcLink", "temporary", "set network as temporary");
	commandline().addOption("ArcLink", "restricted", "set network as restricted");
	commandline().addOption("ArcLink", "private", "set network as not shared");
	commandline().addGroup("Mode");
	commandline().addOption("Mode", "dump", "do not update the database and dump resulting XML to stdout");
	commandline().addOption("Mode", "offline", "do not connect to the messaging server and start with an empty inventory");
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Synchro::validateParameters() {
	if( commandline().hasOption("net-start") && !Core::fromString(_net_start, _net_start_str) &&
		!_net_start.fromString(_net_start_str.c_str(), "%Y-%m-%d")) {
		SEISCOMP_ERROR("invalid time format: %s", _net_start_str.c_str());
		exit(1);
	}
	else {
		_net_start = Core::Time(1980, 1, 1);
	}

	if( commandline().hasOption("net-end") && !Core::fromString(_net_end, _net_end_str) &&
		!_net_end.fromString(_net_end_str.c_str(), "%Y-%m-%d")) {
		SEISCOMP_ERROR("invalid time format: %s", _net_end_str.c_str());
		exit(1);
	}
	else {
		_net_end = Core::Time(2100, 1, 1);
	}

	if( commandline().hasOption("temporary") && _net_start_str.empty() ) {
		SEISCOMP_ERROR("start time must be specified for temporary networks");
		exit(1);
	}

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
	
	try { _dcid = configGetString("datacenterID"); } catch (...) {}

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

	if(_dcid.empty()) {
		cout << "Please specify datacenter/archive ID" << endl;
		return false;
	}

	_syncID = name() + "_" + Core::Time::GMT().iso();

	Dataless *dl = new Dataless(_dcid, _net_description, _net_type, _net_start, _net_end,
		commandline().hasOption("temporary"), commandline().hasOption("restricted"),
		!commandline().hasOption("private"), commandline().hasOption("dump"));
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

