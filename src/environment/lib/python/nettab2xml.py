#***************************************************************************** 
# nettab2xml.py
#
# Convert network tables to XML
#
# (c) 2009 Andres Heinloo, GFZ Potsdam
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any later
# version. For more information, see http://www.gnu.org/
#*****************************************************************************

import re
import sys
import csv
import time
import datetime
import os.path
import decimal
import fnmatch
import copy
from decimal import Decimal
from seiscomp import logs
from seiscomp.db.seiscomp3 import sc3wrap
from seiscomp.db.seiscomp3.routing import Routing as SC3Routing
from seiscomp.db.seiscomp3.inventory import Inventory as SC3Inventory
from seiscomp.nettab import Nettab, Instruments, NettabError, _EPOCH_DATE
from seiscomp3 import Core, Client, DataModel, Logging, Config
from seiscomp3.helpers import writeobj

VERSION = "0.1 (2010.242)"
    
_rx_route = re.compile(r'([^,(]+)(\(([^)]*)\))?,?')

class SyncNettab(Client.Application):
    def __init__(self, argc, argv):
        Client.Application.__init__(self, argc, argv)
    
        self.dcid = None
        self.inst_db_file = None
        self.stat_map_file = None
        self.access_net_file = None
        self.access_stat_file = None
        self.sensor_attr_file = None
        self.datalogger_attr_file = None
        self.network_attr_file = None
        self.station_attr_file = None
        self.tab_files = None

        self.setLoggingToStdErr(True)
        self.setMessagingEnabled(False)
        self.setDatabaseEnabled(False, False)
        self.setAutoApplyNotifierEnabled(False)
        self.setInterpretNotifierEnabled(False)
    
    def createCommandLineDescription(self):
        Client.Application.createCommandLineDescription(self)

        self.commandline().addGroup("ArcLink")
        self.commandline().addStringOption("ArcLink", "dcid", "datacenter/archive ID")
        self.commandline().addStringOption("ArcLink", "inst-db", "path to inst.db")
        self.commandline().addStringOption("ArcLink", "stat-map", "path to stat_map.lst")
        self.commandline().addStringOption("ArcLink", "access-net", "path to access.net")
        self.commandline().addStringOption("ArcLink", "access-stat", "path to access.stat")
        self.commandline().addStringOption("ArcLink", "sensor-attr", "path to sensor_attr.csv")
        self.commandline().addStringOption("ArcLink", "datalogger-attr", "path to datalogger_attr.csv")
        self.commandline().addStringOption("ArcLink", "network-attr", "path to network_attr.csv")
        self.commandline().addStringOption("ArcLink", "station-attr", "path to station_attr.csv")

    def validateParameters(self):
        try:
            if self.commandline().hasOption("dcid"):
                self.dcid = self.commandline().optionString("dcid")

            if self.commandline().hasOption("stat-map"):
                self.stat_map_file = self.commandline().optionString("stat-map")

            if self.commandline().hasOption("access-net"):
                self.access_net_file = self.commandline().optionString("access-net")

            if self.commandline().hasOption("access-stat"):
                self.access_stat_file = self.commandline().optionString("access-stat")

            if self.commandline().hasOption("sensor-attr"):
                self.sensor_attr_file = self.commandline().optionString("sensor-attr")

            if self.commandline().hasOption("datalogger-attr"):
                self.datalogger_attr_file = self.commandline().optionString("datalogger-attr")

            if self.commandline().hasOption("network-attr"):
                self.network_attr_file = self.commandline().optionString("network-attr")

            if self.commandline().hasOption("station-attr"):
                self.station_attr_file = self.commandline().optionString("station-attr")
            
            if self.commandline().hasOption("inst-db"):
                self.inst_db_file = self.commandline().optionString("inst-db")

            else:
                print >>sys.stderr, "Please specify location of inst.db using --inst-db"
                return False

            self.tab_files = self.commandline().unrecognizedOptions()

        except Exception:
            logs.print_exc()
            return False

        return True
    
    def initConfiguration(self):
        if not Client.Application.initConfiguration(self):
            return False
 
        # force logging to stderr even if logging.file = 1
        self.setLoggingToStdErr(True)

        try:
            self.dcid = self.configGetString("datacenterID")

        except Config.ConfigException:
            pass

        return True
        
    def __load_file(self, func, file):
        if file:
            logs.info("loading " + file)
            func(file)

    def run(self):
        try:
            if self.dcid is None:
                print >>sys.stderr, "Please specify datacenter/archive ID"
                return False
            
            nettab = Nettab(self.dcid)
            instdb = Instruments(self.dcid)

            try:
                self.__load_file(instdb.load_db, self.inst_db_file)
                self.__load_file(nettab.load_statmap, self.stat_map_file)
                self.__load_file(nettab.load_access_net, self.access_net_file)
                self.__load_file(nettab.load_access_stat, self.access_stat_file)
                self.__load_file(instdb.load_sensor_attr, self.sensor_attr_file)
                self.__load_file(instdb.load_datalogger_attr, self.datalogger_attr_file)
                self.__load_file(nettab.load_network_attr, self.network_attr_file)
                self.__load_file(nettab.load_station_attr, self.station_attr_file)

                for tab in self.tab_files:
                    self.__load_file(nettab.load_tab, tab)
                    inv = SC3Inventory(DataModel.Inventory())
                    nettab.update_inventory(instdb, inv)
                    if tab.endswith(".tab"):
                        xmlfile = os.path.basename(tab[:-4] + ".xml")
                    else:
                        xmlfile = os.path.basename(tab + ".xml")

                    writeobj(inv.obj, xmlfile)

            except (IOError, NettabError), e:
                logs.error("fatal error: " + str(e))
                return False

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
    app = SyncNettab(len(sys.argv), sys.argv)
    sys.exit(app())

