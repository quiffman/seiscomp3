#***************************************************************************** 
# fill_db.py
#
# Fill database using inventory data in XML format
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
import time
import datetime
from seiscomp import logs
from seiscomp.db.generic.routing import Routing as GRouting
from seiscomp.db.generic.inventory import Inventory as GInventory
from seiscomp3 import Core, Client, DataModel, Logging

try:
    from seiscomp.db.seiscomp3 import sc3wrap
    from seiscomp.db.seiscomp3.routing import Routing as SC3Routing
    from seiscomp.db.seiscomp3.inventory import Inventory as SC3Inventory
    have_sc3wrap = True

except ImportError:
    have_sc3wrap = False

try:
    import sqlobject
    from seiscomp.db.sqlobject.routing import Routing as SORouting
    from seiscomp.db.sqlobject.inventory import Inventory as SOInventory
    have_sqlobject = True

except ImportError:
    have_sqlobject = False

VERSION = "1.0 (2012.048)"

class FillDB(Client.Application):
    def __init__(self, argc, argv):
        Client.Application.__init__(self, argc, argv)
    
        self.use_sc3db = have_sc3wrap
        self.db_url = None
        self.seedlink_addr = None
        self.arclink_addr = None
        self.input_file = None

        self.setLoggingToStdErr(True)
        
        self.setMessagingEnabled(have_sc3wrap)
        self.setDatabaseEnabled(have_sc3wrap, have_sc3wrap)
    
        self.setAutoApplyNotifierEnabled(False)
        self.setInterpretNotifierEnabled(False)
    
        self.setPrimaryMessagingGroup("LISTENER_GROUP")

    def createCommandLineDescription(self):
        Client.Application.createCommandLineDescription(self)

        self.commandline().addGroup("ArcLink")
        self.commandline().addIntOption("ArcLink", "use-sc3db", "use SC3 messaging/database", have_sc3wrap)
        self.commandline().addStringOption("ArcLink", "db-url", "database URL (sqlobject only)")
        self.commandline().addStringOption("ArcLink", "arclink", "public arclink address for routing")
        self.commandline().addStringOption("ArcLink", "seedlink", "public seedlink address for routing")
        
    def validateParameters(self):
        try:
            if self.commandline().hasOption("use-sc3db"):
                self.use_sc3db = self.commandline().optionInt("use-sc3db")

            if self.commandline().hasOption("db-url"):
                self.db_url = self.commandline().optionString("db-url")

            if self.commandline().hasOption("arclink"):
                self.arclink_addr = self.commandline().optionString("arclink")

            if self.commandline().hasOption("seedlink"):
                self.seedlink_addr = self.commandline().optionString("seedlink")

            if not have_sc3wrap and not have_sqlobject:
                print >>sys.stderr, "Neither SC3 nor sqlobject support is available"
                return False

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
                print >>sys.stderr, "Usage: fill_db [options] file"
                return False

            self.input_file = args[0]

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
        
    def send(self, *args):
        while not self.connection().send(*args):
            logs.warning("send failed, retrying")
            time.sleep(1)
    
    def send_notifiers(self, group):
        Nsize = DataModel.Notifier.Size()

        if Nsize > 0:
            logs.info("trying to apply %d changes..." % Nsize)
        else:
            logs.info("no changes to apply")
            return

        Nmsg = DataModel.Notifier.GetMessage(True)
            
        it = Nmsg.iter()
        msg = DataModel.NotifierMessage()

        maxmsg = 100
        sent = 0
        mcount = 0

        try:
            while it.get():
                msg.attach(DataModel.Notifier_Cast(it.get()))
                mcount += 1
                if msg and mcount == maxmsg:
                    sent += mcount
                    logs.debug("sending message (%5.1f %%)" % (sent / float(Nsize) * 100.0))
                    self.send(group, msg)
                    msg.clear()
                    mcount = 0
                it.next()
        except:
            pass

        finally:
            if msg:
                logs.debug("sending message (%5.1f %%)" % 100.0)
                self.send(group, msg)
                msg.clear()

    def run(self):
        try:
            if self.use_sc3db:
                sc3wrap.dbQuery = self.query()
                DataModel.Notifier.Enable()
                DataModel.Notifier.SetCheckEnabled(False)
                self.inv = SC3Inventory(self.query().loadInventory())
                self.rtn = SC3Routing(self.query().loadRouting())
            
            else:
                connection = sqlobject.connectionForURI(self.db_url)
                sqlobject.sqlhub.processConnection = connection
                self.inv = SOInventory()
                self.rtn = SORouting()

            self.inv.load_stations("*")
            self.inv.load_stations("*", "*")
            self.inv.load_stations("*", "*", "*")
            self.inv.load_stations("*", "*", "*", "*")
            self.inv.load_instruments()
            self.inv.load_xml(self.input_file)
            self.inv.flush()

            if self.use_sc3db:
                self.send_notifiers("INVENTORY")

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
    app = FillDB(len(sys.argv), sys.argv)
    sys.exit(app())

