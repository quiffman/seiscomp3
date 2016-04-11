#define DLSV2INV
/*=====================================================================
    Name:	synchro.C

    Purpose:  	synchronisation between stationdb and seiscompdb

    Usage:   	will be called from script monitor.sh, 
		that is sheduled from synchronistation

    Language:   C++, ANSI standard.

    Author:     Peter de Boer, KNMI, Netherlands

    Revision:	2007-11-16	0.1	initial

=====================================================================*/
#include "synchro.h"
#include "dataless.h"

using namespace std;

namespace Seiscomp {
namespace Applications {


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Synchro::Synchro(int argc, char **argv) : Client::Application(argc, argv) {
	setLoggingToStdErr(true);
#ifdef DLSV2INV
	setMessagingEnabled(false);
	setDatabaseEnabled(false, false);
	setLoadInventoryEnabled(false);
#else
	setMessagingEnabled(true);
	setDatabaseEnabled(true, true);
	setLoadInventoryEnabled(true);
#endif
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
	commandline().addOption<string>("ArcLink", "dcid", "datacenter/archive ID", &_dcid, false);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Synchro::validateParameters() {
	if( commandline().hasOption("dcid") )
		_init[ARCHIVE] = _dcid;

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

	Dataless *dl = new Dataless(_init);
	dl->SynchronizeDataless(args[0], connection());

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


} // namespace Applications
} // namespace Seiscomp

