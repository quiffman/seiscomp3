#!/usr/bin/env python

############################################################################
#    Copyright (C) by GFZ Potsdam                                          #
#                                                                          #
#    You can redistribute and/or modify this program under the             #
#    terms of the SeisComP Public License.                                 #
#                                                                          #
#    This program is distributed in the hope that it will be useful,       #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#    SeisComP Public License for more details.                             #
############################################################################

import time, sys, os, time
import seiscomp3.Client
from scbulletin import Bulletin, stationCount

# FIXME never cleaned up, but that shouldn't be a big problem
procAlertEventIDs = {}

class ProcAlert(seiscomp3.Client.Application):
    def __init__(self, argc, argv):
        seiscomp3.Client.Application.__init__(self, argc, argv)
    
        self.setMessagingEnabled(True)
        self.setDatabaseEnabled(True, True)
    
        self.setAutoApplyNotifierEnabled(True)
        self.setInterpretNotifierEnabled(True)
    
        self.setPrimaryMessagingGroup("LISTENER_GROUP")
        self.addMessagingSubscription("EVENT")
        self.addMessagingSubscription("LOCATION")
        self.addMessagingSubscription("MAGNITUDE")
    
        self.maxAgeDays = 1.
        self.minPickCount = 25

        self.procAlertScript = ""

        ep = seiscomp3.DataModel.EventParameters()


    def createCommandLineDescription(self):
        try:
            self.commandline().addGroup("Publishing")
            self.commandline().addIntOption("Publishing", "min-arr", "Minimum arrival count of a published origin", self.minPickCount)
            self.commandline().addDoubleOption("Publishing", "max-age", "Maximum age in days of published origins", self.maxAgeDays)
            self.commandline().addStringOption("Publishing", "procalert-script", "Specify the script to publish an event. The ProcAlert file and the event id are passed as parameter $1 and $2")
            self.commandline().addOption("Publishing", "test", "Test mode, no messages are sent")
        except:
            seiscomp3.Logging.warning("caught unexpected error %s" % sys.exc_info())


    def initConfiguration(self):
        if not seiscomp3.Client.Application.initConfiguration(self): return False

        try: self.procAlertScript = self.configGetString("scripts.procAlert")
        except: pass

        try: self.minPickCount = self.configGetInt("minArrivals")
        except: pass

        try: self.maxAgeDays = self.configGetDouble("maxAgeDays")
        except: pass

        return True


    def init(self):
        if not seiscomp3.Client.Application.init(self): return False

        try: self.procAlertScript = self.commandline().optionString("procalert-script")
        except: pass

        try: self.minPickCount = self.commandline().optionInt("min-arr")
        except: pass

        try: self.maxAgeDays = self.commandline().optionDouble("max-age")
        except: pass

        self.bulletin = Bulletin(self.query(), "autoloc1")
        self.cache = seiscomp3.DataModel.PublicObjectRingBuffer(self.query(), 100)

        if not self.procAlertScript:
            seiscomp3.Logging.warning("No procalert script given")
        else:
            seiscomp3.Logging.info("Using procalert script: %s" % self.procAlertScript)

        return True


    def addObject(self, parentID, obj):
        org = seiscomp3.DataModel.Origin.Cast(obj)
        if org:
            self.cache.feed(org)
            seiscomp3.Logging.info("Received origin %s" % org.publicID())
            return

        self.updateObject(parentID, obj)


    def updateObject(self, parentID, obj):
        try:
            evt = seiscomp3.DataModel.Event.Cast(obj)
            if evt:
                orid = evt.preferredOriginID()
        
                org = self.cache.get(seiscomp3.DataModel.Origin, orid)
                if not org:
                    seiscomp3.Logging.error("Unable to fetch origin %s" % orid)
                    return

                if org.arrivalCount() == 0:
                    self.query().loadArrivals(org)
                if org.stationMagnitudeCount() == 0:
                    self.query().loadStationMagnitudes(org)
                if org.networkMagnitudeCount() == 0:
                    self.query().loadNetworkMagnitudes(org)

                if not self.originMeetsCriteria(org, evt):
                    seiscomp3.Logging.warning("Origin %s not published" % orid)
                    return

                txt = self.bulletin.printEvent(evt)
        
                for line in txt.split("\n"):
                    line = line.rstrip()
                    seiscomp3.Logging.info(line)
                seiscomp3.Logging.info("")

                if not self.commandline().hasOption("test"):
                    evid = evt.publicID()
                    if evid[:5] != "gfz20": # new-style event id?
                        evid = self._oldID(evt)
                    self.send_procalert(txt, evid)

                return

        except:
            seiscomp3.Logging.warning("caught unexpected error %s" % sys.exc_info())


    def _oldID(self, evt):

        evid = evt.publicID()

        if evid not in procAlertEventIDs:
            if evid[:2] == "ev" and evid[2].isdigit() and evid[3].isdigit():
                n = evid.find("#")
                if n == -1:
                    procAlertEventIDs[evid] = evid
                else:
                    procAlertEventIDs[evid] = evid[:n]
            else:
                try:
                    t = evt.created()
                except:
                    t = seiscomp3.Core.Time.GMT()
                procAlertEventIDs[evid] = "ev%s" % t.toString("%y%m%d%H%M%S")

        return procAlertEventIDs[evid]


    def hasValidNetworkMagnitude(self, org, evt):
        nmag = org.networkMagnitudeCount()
        for imag in xrange(nmag):
            mag = org.networkMagnitude(imag)
            if mag.publicID() == evt.preferredMagnitudeID():
                return True
        return False


    def send_procalert(self, txt, evid):
        if self.procAlertScript:
            if evid[:5] == "gfz20":
                tmp = "/tmp/yyy%s" % evid[3:]
            else:
                tmp = "/tmp/yy%s"  % evid[2:]
            f = file(tmp, "w")
            f.write("%s" % txt)
            f.close()
        
            os.system(self.procAlertScript + " " + tmp + " " + tmp[6:])


    def coordinates(self, org):
        return org.latitude().value(), org.longitude().value(), org.depth().value()


    def originMeetsCriteria(self, org, evt):
        publish = True

        lat, lon, dep = self.coordinates(org)

        if 43 < lat < 70 and -10 < lon < 60 and dep > 200:
            seiscomp3.Logging.error("suspicious region/depth - ignored")
            publish = False

        if stationCount(org) < self.minPickCount:
            seiscomp3.Logging.error("too few picks - ignored")
            publish = False

        now = seiscomp3.Core.Time.GMT()
        if (now - org.time().value()).seconds()/86400. > self.maxAgeDays:
            seiscomp3.Logging.error("origin too old - ignored")
            publish = False

        if org.status() in [ seiscomp3.DataModel.MANUAL_ORIGIN, seiscomp3.DataModel.CONFIRMED ]:
        #if org.status() in [ seiscomp3.DataModel.CONFIRMED ]:
            publish = True

        if not self.hasValidNetworkMagnitude(org, evt):
            seiscomp3.Logging.error("no network magnitude - ignored")
            publish = False

        return publish


app = ProcAlert(len(sys.argv), sys.argv)
sys.exit(app())
