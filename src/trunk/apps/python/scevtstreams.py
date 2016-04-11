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

import sys
import seiscomp3.Client

class EventStreams(seiscomp3.Client.Application):
  def __init__(self, argc, argv):
    seiscomp3.Client.Application.__init__(self, argc, argv)

    self.setMessagingEnabled(False)
    self.setDatabaseEnabled(True, False)
    self.setDaemonEnabled(False)

    self.eventID = None
    self.margin = 300

    self.allComponents = True
    self.allLocations = True

    self.streams = []


  def createCommandLineDescription(self):
    self.commandline().addGroup("Dump")
    self.commandline().addStringOption("Dump", "event,E", "event id")
    self.commandline().addIntOption("Dump", "margin,m", "time margin around the picked timewindow, default is 300")
    self.commandline().addStringOption("Dump", "streams,S", "comma separated list of streams per station to add, e.g. BH,SH,HH")
    self.commandline().addIntOption("Dump", "all-components,C", "all components or just the picked one, default is True")
    self.commandline().addIntOption("Dump", "all-locations,L", "all components or just the picked one, default is True")
    return True


  def init(self):
    try:
      if not seiscomp3.Client.Application.init(self): return False

      try:
        self.eventID = self.commandline().optionString("event")
      except:
        sys.stderr.write("An eventID is mandatory")
        return False

      try:
        self.margin = self.commandline().optionInt("margin")
      except: pass

      try:
        self.streams = self.commandline().optionString("streams").split(",")
      except: pass

      try:
        self.allComponents = self.commandline().optionInt("all-components") != 0
      except: pass

      try:
        self.allLocations = self.commandline().optionInt("all-locations") != 0
      except: pass

      return True
    except:
      cla, exc, trbk = sys.exc_info()
      print cla.__name__
      print exc.__dict__["args"]
      print traceback.format_tb(trbk, 5)


  def run(self):
    try:
      picks = []
      minTime = None
      maxTime = None

      for obj in self.query().getEventPicks(self.eventID):
        pick = seiscomp3.DataModel.Pick.Cast(obj)
        if pick is None: continue
        picks.append(pick)

        if minTime is None: minTime = pick.time().value()
        elif minTime > pick.time().value(): minTime = pick.time().value()

        if maxTime is None: maxTime = pick.time().value()
        elif maxTime < pick.time().value(): maxTime = pick.time().value()

      if minTime: minTime = minTime - seiscomp3.Core.TimeSpan(self.margin)
      if maxTime: maxTime = maxTime + seiscomp3.Core.TimeSpan(self.margin)

      for pick in picks:
        sys.stdout.write(minTime.toString("%F %T") + ";")
        sys.stdout.write(maxTime.toString("%F %T") + ";")
        sys.stdout.write(pick.waveformID().networkCode() + ".")
        sys.stdout.write(pick.waveformID().stationCode() + ".")
        loc = pick.waveformID().locationCode()
        stream = pick.waveformID().channelCode()
        rawStream = stream[:2]

        if self.allComponents == True:
          stream = rawStream + "?"

        if self.allLocations == True:
          loc = ""

        sys.stdout.write(loc + ".")
        sys.stdout.write(stream)

        sys.stdout.write("\n")

        for s in self.streams:
          if s != rawStream:
            sys.stdout.write(minTime.toString("%F %T") + ";")
            sys.stdout.write(maxTime.toString("%F %T") + ";")
            sys.stdout.write(pick.waveformID().networkCode() + ".")
            sys.stdout.write(pick.waveformID().stationCode() + ".")
            sys.stdout.write(loc + ".")
            sys.stdout.write(s + stream[2])

            sys.stdout.write("\n")

      return True
    except:
      cla, exc, trbk = sys.exc_info()
      print cla.__name__
      print exc.__dict__["args"]
      print traceback.format_tb(trbk, 5)


app = EventStreams(len(sys.argv), sys.argv)
sys.exit(app())

