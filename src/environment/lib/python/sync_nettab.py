#***************************************************************************** 
# sync_nettab.py
#
# Load network tables into database
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
from decimal import Decimal
from seiscomp import logs
from seiscomp.fseed import *
from seiscomp.db.generic.routing import Routing as GRouting
from seiscomp.db.generic.inventory import Inventory as GInventory
from seiscomp.nettab import Nettab, Instruments, NettabError, _EPOCH_DATE
from seiscomp3 import Core, Client, DataModel, Logging, Config

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

VERSION = "1.0 (2010.256)"
    
_rx_route = re.compile(r'([^,(]+)(\(([^)]*)\))?,?')

def _parse_route(s, r):
    prio = 1
    pos = 0
    while pos < len(s):
        m = _rx_route.match(s, pos)
        if m == None:
            print >>sys.stderr, "error parsing route at '" + s[pos:] + "'"
            return False

        if m.group(2):
            pat = tuple((m.group(3).split(',') + ['', '', '', ''])[:4])

        else:
            pat = ('*', '', '', '')

        r.append((m.group(1), prio, pat))
        prio += 1
        pos = m.end()

    return True

class SyncNettab(Client.Application):
    def __init__(self, argc, argv):
        Client.Application.__init__(self, argc, argv)
    
        self.use_sc3db = have_sc3wrap
        self.db_url = None
        self.dcid = None
        self.seedlink_addr = []
        self.arclink_addr = []
        self.inst_db_file = None
        self.stat_map_file = None
        self.access_net_file = None
        self.access_stat_file = None
        self.sensor_attr_file = None
        self.datalogger_attr_file = None
        self.network_attr_file = None
        self.station_attr_file = None
        self.tab_files = None
        self.output_inventory = None
        self.output_routing = None
        self.output_dataless = None
        self.cleanup_routing = 0

        self.setLoggingToStdErr(True)
        
        self.setMessagingEnabled(have_sc3wrap)
        self.setDatabaseEnabled(have_sc3wrap, have_sc3wrap)
    
        self.setAutoApplyNotifierEnabled(False)
        self.setInterpretNotifierEnabled(False)
    
        self.setMessagingUsername("syncnettab")
        self.setPrimaryMessagingGroup("LISTENER_GROUP")

    def createCommandLineDescription(self):
        Client.Application.createCommandLineDescription(self)

        self.commandline().addGroup("ArcLink")
        self.commandline().addIntOption("ArcLink", "use-sc3db", "use SC3 messaging/database", have_sc3wrap)
        self.commandline().addStringOption("ArcLink", "db-url", "database URL (sqlobject only)")
        self.commandline().addStringOption("ArcLink", "dcid", "datacenter/archive ID")
        self.commandline().addStringOption("ArcLink", "arclink", "public arclink address for routing")
        self.commandline().addStringOption("ArcLink", "seedlink", "public seedlink address for routing")
        self.commandline().addStringOption("ArcLink", "inst-db", "path to inst.db")
        self.commandline().addStringOption("ArcLink", "stat-map", "path to stat_map.lst")
        self.commandline().addStringOption("ArcLink", "access-net", "path to access.net")
        self.commandline().addStringOption("ArcLink", "access-stat", "path to access.stat")
        self.commandline().addStringOption("ArcLink", "sensor-attr", "path to sensor_attr.csv")
        self.commandline().addStringOption("ArcLink", "datalogger-attr", "path to datalogger_attr.csv")
        self.commandline().addStringOption("ArcLink", "network-attr", "path to network_attr.csv")
        self.commandline().addStringOption("ArcLink", "station-attr", "path to station_attr.csv")
        self.commandline().addStringOption("ArcLink", "output-inventory", "output to file instead of database")
        self.commandline().addStringOption("ArcLink", "output-routing", "output to file instead of database")
        self.commandline().addStringOption("ArcLink", "output-dataless", "output dataless to file")
        self.commandline().addIntOption("ArcLink", "cleanup-routing", "cleanup routing database", self.cleanup_routing)

    def validateParameters(self):
        try:
            if self.commandline().hasOption("use-sc3db"):
                self.use_sc3db = self.commandline().optionInt("use-sc3db")

            if self.commandline().hasOption("db-url"):
                self.db_url = self.commandline().optionString("db-url")

            if self.commandline().hasOption("dcid"):
                self.dcid = self.commandline().optionString("dcid")

            if self.commandline().hasOption("arclink"):
                if not _parse_route(self.commandline().optionString("arclink"),
                    self.arclink_addr):
                    return False

            if self.commandline().hasOption("seedlink"):
                if not _parse_route(self.commandline().optionString("seedlink"),
                    self.seedlink_addr):
                    return False

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
            
            if self.commandline().hasOption("output-inventory"):
                self.output_inventory = self.commandline().optionString("output-inventory")
            
            if self.commandline().hasOption("output-routing"):
                self.output_routing = self.commandline().optionString("output-routing")
            
            if self.commandline().hasOption("output-dataless"):
                self.output_dataless = self.commandline().optionString("output-dataless")
            
            if self.commandline().hasOption("cleanup-routing"):
                self.cleanup_routing = self.commandline().optionInt("cleanup-routing")
            
            if self.commandline().hasOption("inst-db"):
                self.inst_db_file = self.commandline().optionString("inst-db")

            else:
                print >>sys.stderr, "Please specify location of inst.db using --inst-db"
                return False

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
                    self.sync("sync-nettab")
                it.next()
        except:
            pass

        finally:
            if msg.size():
                logs.debug("sending message (%5.1f %%)" % 100.0)
                self.send(group, msg)
                msg.clear()
                self.sync("sync-nettab")

    def __load_file(self, func, file):
        if file:
            logs.info("loading " + file)
            func(file)

    def run(self):
        try:
            if self.dcid is None:
                print >>sys.stderr, "Please specify datacenter/archive ID"
                return False
            
            instdb = Instruments(self.dcid)
            nettab = Nettab(self.dcid)

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
                    if tab.endswith(".vnet"):
                        self.__load_file(nettab.load_vnet, tab)
                    else:
                        self.__load_file(nettab.load_tab, tab)

                if self.output_inventory:
                    logs.info("writing inventory to " + self.output_inventory)
                    inv = SC3Inventory(DataModel.Inventory())
                    nettab.update_inventory(instdb, inv)
                    inv.save_xml(self.output_inventory, instr = 2)

                if self.output_routing:
                    logs.info("writing access rules and routing data to " + self.output_routing)
                    inv = SC3Routing(DataModel.Routing())
                    nettab.update_access(rtn)
                    rtn.save_xml(self.output_routing, use_access = True)

                if self.output_dataless:
                    logs.info("writing dataless to " + self.output_dataless)
                    inv = SC3Inventory(DataModel.Inventory())
                    nettab.update_inventory(instdb, inv)
                    vol = SEEDVolume(inv, "WebDC", "SeisComP SEED Volume", resp_dict=False)

                    for net in sum([i.values() for i in inv.network.itervalues()], []):
                        for sta in sum([i.values() for i in net.station.itervalues()], []):
                            for loc in sum([i.values() for i in sta.sensorLocation.itervalues()], []):
                                  for strm in sum([i.values() for i in loc.stream.itervalues()], []):
                                       try:
                                           vol.add_chan(net.code, sta.code, loc.code, strm.code, strm.start, strm.end)

                                       except SEEDError, e:
                                           print "Error (%s,%s,%s,%s):" % (net.code, sta.code, loc.code, strm.code), str(e)
                    
                    try:
                        vol.output(self.output_dataless)

                    except SEEDError, e:
                        print "Error:", str(e)

            except (IOError, NettabError), e:
                logs.error("fatal error: " + str(e))
                return False

            if self.output_inventory or self.output_routing or self.output_dataless:
                return True
            
            logs.info("loading database")

            if self.use_sc3db:
                sc3wrap.dbQuery = self.query()
                DataModel.Notifier.Enable()
                DataModel.Notifier.SetCheckEnabled(False)
                inv = SC3Inventory(self.query().loadInventory())
                rtn = SC3Routing(self.query().loadRouting())
            
            else:
                connection = sqlobject.connectionForURI(self.db_url)
                sqlobject.sqlhub.processConnection = connection
                inv = SOInventory()
                rtn = SORouting()

            logs.info("updating inventory")

            inv.load_stations("*")
            inv.load_stations("*", "*")
            inv.load_stations("*", "*", "*")
            inv.load_stations("*", "*", "*", "*")
            inv.load_instruments()

            try:
                nettab.update_inventory(instdb, inv)

            except NettabError, e:
                logs.error("fatal error: " + str(e))
                return False
                
            inv.flush()

            if self.use_sc3db:
                self.send_notifiers("INVENTORY")

            logs.info("updating access rules")
            
            rtn.load_access()
            
            try:
                nettab.update_access(rtn)

            except NettabError, e:
                logs.error("fatal error: " + str(e))
                return False
                
            rtn.flush()

            if self.use_sc3db:
                self.send_notifiers("ROUTING")

            logs.info("updating routing data")

            rtn.load_routes()

            arcl_addr = {}
            seedl_addr = {}

            for net in sum([i.values() for i in inv.network.itervalues()], []):
                for sta in sum([i.values() for i in net.station.itervalues()], []):
                    if sta.archive:
                        if sta.archive != self.dcid:
                            continue
                    elif net.archive != self.dcid:
                        continue
                 
                    for loc in sum([i.values() for i in sta.sensorLocation.itervalues()], []):
                        for strm in sum([i.values() for i in loc.stream.itervalues()], []) + \
                            sum([i.values() for i in loc.auxStream.itervalues()], []):
                            for (addr, prio, (net_code, sta_code, strm_code, loc_code)) in self.arclink_addr:
                                if fnmatch.fnmatchcase(net.code, net_code):
                                    net_code = net.code

                                elif net_code:
                                    continue

                                if fnmatch.fnmatchcase(sta.code, sta_code):
                                    sta_code = sta.code

                                elif sta_code:
                                    continue

                                if fnmatch.fnmatchcase(loc.code, loc_code):
                                    loc_code = loc.code

                                elif loc_code:
                                    continue

                                if fnmatch.fnmatchcase(strm.code, strm_code):
                                    strm_code = strm.code

                                elif strm_code:
                                    continue

                                try:
                                    rt = rtn.route[net_code][sta_code][loc_code][strm_code]

                                except KeyError:
                                    rt = rtn.insert_route(net_code, sta_code, loc_code, strm_code)

                                try:
                                    arcl = rt.arclink[addr][_EPOCH_DATE]
                                    arcl.priority = prio

                                except KeyError:
                                    rt.insert_arclink(addr, _EPOCH_DATE, priority=prio)

                                try:
                                    addr_set = arcl_addr[(net_code, sta_code, loc_code, strm_code)]

                                except KeyError:
                                    addr_set = set()
                                    arcl_addr[(net_code, sta_code, loc_code, strm_code)] = addr_set
                                    
                                addr_set.add(addr)

                            for (addr, prio, (net_code, sta_code, strm_code, loc_code)) in self.seedlink_addr:
                                if fnmatch.fnmatchcase(net.code, net_code):
                                    net_code = net.code

                                elif net_code:
                                    continue

                                if fnmatch.fnmatchcase(sta.code, sta_code):
                                    sta_code = sta.code

                                elif sta_code:
                                    continue

                                if fnmatch.fnmatchcase(loc.code, loc_code):
                                    loc_code = loc.code

                                elif loc_code:
                                    continue

                                if fnmatch.fnmatchcase(strm.code, strm_code):
                                    strm_code = strm.code

                                elif strm_code:
                                    continue

                                try:
                                    rt = rtn.route[net_code][sta_code][loc_code][strm_code]

                                except KeyError:
                                    rt = rtn.insert_route(net_code, sta_code, loc_code, strm_code)

                                try:
                                    seedl = rt.seedlink[addr]
                                    seedl.priority = prio

                                except KeyError:
                                    rt.insert_seedlink(addr, priority=prio)

                                try:
                                    addr_set = seedl_addr[(net_code, sta_code, loc_code, strm_code)]

                                except KeyError:
                                    addr_set = set()
                                    seedl_addr[(net_code, sta_code, loc_code, strm_code)] = addr_set
                                    
                                addr_set.add(addr)

            if self.cleanup_routing > 0:
                for rt_net in rtn.route.values():
                    for rt_sta in rt_net.values():
                        for rt_loc in rt_sta.values():
                            for rt in rt_loc.values():
                                if self.cleanup_routing < 2:
                                    net_tp = inv.network.get(rt.networkCode)
                                    if not net_tp:
                                        continue

                                    dcid_set = set([ x.archive for x in net_tp.itervalues() ])
                                    if self.dcid not in dcid_set:
                                        continue

                                try:
                                    addr_set = arcl_addr[(rt.networkCode, rt.stationCode, rt.locationCode, rt.streamCode)]

                                except KeyError:
                                    addr_set = set()

                                for arcl in sum([i.values() for i in rt.arclink.itervalues()], []):
                                    if arcl.address not in addr_set or \
                                        arcl.start != _EPOCH_DATE:
                                        rt.remove_arclink(arcl.address, arcl.start)
                                    
                                try:
                                    addr_set = seedl_addr[(rt.networkCode, rt.stationCode, rt.locationCode, rt.streamCode)]

                                except KeyError:
                                    addr_set = set()

                                for seedl in rt.seedlink.values():
                                    if seedl.address not in addr_set:
                                        rt.remove_seedlink(seedl.address)

                                if not rt.arclink and not rt.seedlink:
                                    rtn.remove_route(rt.networkCode, rt.stationCode, rt.locationCode, rt.streamCode)
                        
            rtn.flush()
            
            if self.use_sc3db:
                self.send_notifiers("ROUTING")

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

