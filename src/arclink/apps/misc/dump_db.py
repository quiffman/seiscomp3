#!/usr/bin/env python
#***************************************************************************** 
# dump_db.py
#
# Dump inventory database in XML format
#
# (c) 2006 Andres Heinloo, GFZ Potsdam
# (c) 2007 Mathias Hoffmann, GFZ Potsdam
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any later
# version. For more information, see http://www.gnu.org/
#*****************************************************************************

import sys
from seiscomp import logs
from seiscomp.db.generic.inventory import Inventory as GInventory
from seiscomp3 import Core, Client, DataModel, Logging

try:
    from seiscomp.db.seiscomp3 import sc3wrap
    from seiscomp.db.seiscomp3.inventory import Inventory as SC3Inventory
    from seiscomp.db.seiscomp3.routing import Routing as SC3Routing
    have_sc3wrap = True

except ImportError:
    have_sc3wrap = False

try:
    import sqlobject
    from seiscomp.db.sqlobject.inventory import Inventory as SOInventory
    from seiscomp.db.sqlobject.routing import Routing as SORouting
    have_sqlobject = True

except ImportError:
    have_sqlobject = False

VERSION = "1.1 (2012.165)"

class DumpDB(Client.Application):
    def __init__(self, argc, argv):
        Client.Application.__init__(self, argc, argv)
        self.routingMode = False
        self.addAccess = False
    
        self.use_sc3db = have_sc3wrap
        self.db_url = None
        self.output_file = None

        self.setLoggingToStdErr(True)

        self.setMessagingEnabled(True)
        self.setDatabaseEnabled(have_sc3wrap, have_sc3wrap)
    
        self.setAutoApplyNotifierEnabled(False)
        self.setInterpretNotifierEnabled(False)
    
        self.setPrimaryMessagingGroup("LISTENER_GROUP")

    def createCommandLineDescription(self):
        Client.Application.createCommandLineDescription(self)

        self.commandline().addGroup("ArcLink")
        self.commandline().addIntOption("ArcLink", "use-sc3db", "use SC3 messaging/database", have_sc3wrap)
        self.commandline().addStringOption("ArcLink", "db-url", "database URL (sqlobject only)")
        self.commandline().addOption("ArcLink", "routing", "dump routing instead of inventory")
        self.commandline().addOption("ArcLink", "with-access", "dump access together with routing information")
        
    def validateParameters(self):
        try:
            if self.commandline().hasOption("use-sc3db"):
                self.use_sc3db = self.commandline().optionInt("use-sc3db")

            if self.commandline().hasOption("db-url"):
                self.db_url = self.commandline().optionString("db-url")

            if not have_sc3wrap and not have_sqlobject:
                print >>sys.stderr, "Neither SC3 nor sqlobject support is available"
                return False

            if self.commandline().hasOption("routing"):
                self.routingMode = True

            if self.commandline().hasOption("with-access"):
                self.addAccess = True

            if self.use_sc3db:
                if not have_sc3wrap:
                    print >>sys.stderr, "SC3 database support is not available"
                    return False

            else:
                if not have_sqlobject:
                    print >>sys.stderr, "sqlobject support not is available"
                    return False

                if self.db_url is None:
                    print >>sys.stderr, "Please specify database URL using --db-url"
                    return False

            args = self.commandline().unrecognizedOptions()
            if len(args) != 1:
                print >>sys.stderr, "Usage: dump_db [options] file"
                return False

            self.output_file = args[0]

        except Exception:
            logs.print_exc()
            return False

        return True
    
    def initConfiguration(self):
        if not Client.Application.initConfiguration(self):
            return False
 
        # force logging to stderr even if logging.file = 1
        self.setLoggingToStdErr(True)

        return True
        
    def run(self):
        try:
            if self.use_sc3db:
                sc3wrap.dbQuery = self.query()
                DataModel.Notifier.Enable()
                DataModel.Notifier.SetCheckEnabled(False)
                if not self.routingMode:
                    self.inv = SC3Inventory(self.query().loadInventory())
                else:
                    self.rtn = SC3Routing(self.query().loadRouting())
            
            else:
                connection = sqlobject.connectionForURI(self.db_url)
                sqlobject.sqlhub.processConnection = connection
                if not self.routingMode:
                    self.inv = SOInventory()
                else:
                    self.rtn = SORouting()

            if not self.routingMode:
                self.inv.load_stations("*")
                self.inv.load_stations("*", "*")
                self.inv.load_stations("*", "*", "*")
                self.inv.load_stations("*", "*", "*", "*")
                self.inv.load_instruments()
                self.inv.save_xml(self.output_file, instr=2)
            else:
                self.rtn.load_routes("*", "*")
                if self.addAccess:
                    self.rtn.load_access("*")
                    self.rtn.load_access("*", "*")
                    self.rtn.load_access("*", "*", "*")
                    self.rtn.load_access("*", "*", "*", "*")
                self.rtn.save_xml(self.output_file, self.addAccess)

        except Exception:
            logs.print_exc()
            return False

        return True

if __name__ == "__main__":
    logs.debug = Logging.debug
    logs.info = Logging.info
    logs.notice = Logging.notice
    logs.warning = Logging.warning
    logs.error = Logging.error
    app = DumpDB(len(sys.argv), sys.argv)
    sys.exit(app())

