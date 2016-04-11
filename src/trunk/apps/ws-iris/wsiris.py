#!/usr/bin/env python

################################################################################
# Copyright (C) 2012 by gempa GmbH
#
# WSIris -- Implements IRIS Web service interface, see http://www.iris.edu/ws
#
# Implemented Services:
#   ws-availability
#   ws-dataselect
#   ws-bulkdataselect
#   ws-event
#   ws-station
#
# Author:  Stephan Herrnkind
# Email:   herrnkind@gempa.de
################################################################################

import sys
import os
import time

try:
	from twisted.internet import reactor
	from twisted.web import server, static
	from twisted.python import log
	from twisted.web import resource
except ImportError, e:
	sys.exit("%s\nIs python twisted installed?" % str(e))

try:
	from seiscomp3 import Core, DataModel, Logging
	from seiscomp3.Client import Application
	from seiscomp3.System import Environment
	from seiscomp3.Config import Exception as ConfigException
except ImportError, e:
	sys.exit("%s\nIs the SeisComP environment set correctly?" % str(e))

from seiscomp3.wsiris.wsevent import WSEvent
from seiscomp3.wsiris.wsstation import WSStation
from seiscomp3.wsiris.wsavailability import WSAvailability
from seiscomp3.wsiris.wsdataselect import WSDataSelect
from seiscomp3.wsiris.wsbulkdataselect import WSBulkDataSelect
from seiscomp3.wsiris.http import NoResource, Site


class Simple(resource.Resource):
    isLeaf = True
    def render_GET(self, request):
        return "<html>Hello, world!</html>"


def logSC3(entry):
	try:
		isError = entry['isError']
		msg = entry['message']
		if isError:
			for l in msg:
				Logging.error("[reactor] %s" % l)
		else:
			for l in msg:
				Logging.info("[reactor] %s" % l)
	except:
		pass


################################################################################
class WSIris(Application):

	#---------------------------------------------------------------------------
	def __init__(self):
		Application.__init__(self, len(sys.argv), sys.argv)
		self.setMessagingEnabled(False)
		self.setDatabaseEnabled(True, True)

		self._serverRoot    = os.path.dirname(__file__)
		self._listenAddress = "0.0.0.0" # all interfaces
		self._port          = 8080
		self._sdsPath       = os.path.join(Environment.Instance().installDir(),\
		                                   "var", "lib", "archive")
		self._queryObjects  = 10000
		self._connections   = 5

		self._inv           = None

		# Leave signal handling to us
		Application.HandleSignals(False, False)


	#---------------------------------------------------------------------------
	def initConfiguration(self):
		if not Application.initConfiguration(self):
			return False

		cfg = self.configuration()

		# bind address and port
		try: self._listenAddress = cfg.getString("listenAddress")
		except ConfigException: pass
		try: self._port = cfg.getInt("port")
		except ConfigException: pass

		# SDS path
		try: self._sdsPath = Environment.Instance().absolutePath(cfg.getString("sdsPath"))
		except ConfigException: pass

		# maximum number of objects per query, used in ws-station and ws-event
		# services to limit main memory consumption
		try: self._queryObjects = cfg.getInt("queryObjects")
		except ConfigException: pass

		# maximum number of connections
		try: self._connections = cfg.getInt("connections")
		except ConfigException: pass

		return True


	#---------------------------------------------------------------------------
	# Signal handling in Python and fork in wrapped C++ code is not a good
	# combination. Without digging too much into the problem, forking the
	# process with os.fork() helps
	def forkProcess(self):
		cp = os.fork()
		if cp < 0: return False
		elif cp == 0: return True
		elif cp > 0:
			sys.exit(0)


	#---------------------------------------------------------------------------
	def run(self):
		Logging.notice("\n" \
		               "Configuration read:\n" \
		               "  listenAddress: %s\n" \
		               "  port         : %i\n" \
		               "  sdsPath      : %s\n" \
		               "  queryObjects : %i\n" \
		               "  connections  : %i\n" % (
		               self._listenAddress, self._port, self._sdsPath,
		               self._queryObjects, self._connections))

		# load inventory
		self._loadInventory()
		DataModel.PublicObject.SetRegistrationEnabled(False)

		# create resource tree
		root = NoResource()
		fileName = os.path.join(self._serverRoot, "img/favicon.ico")
		root.putChild("favicon.ico", static.File(fileName, "image/x-icon"))

		event = WSEvent()
		event.putChild("query", WSEvent())
		root.putChild("event", event)

		station = WSStation()
		station.putChild("query", WSStation())
		root.putChild("station", station)

		avail = WSAvailability()
		avail.putChild("query", WSAvailability())
		root.putChild("availability", avail)

		select = WSDataSelect()
		select.putChild("query", WSDataSelect())
		root.putChild("dataselect", select)

		bulkselect = WSBulkDataSelect()
		bulkselect.putChild("query", WSBulkDataSelect())
		root.putChild("bulkdataselect", bulkselect)

		site = Site(root)
		site.setVersion(self.version())

		# start listen for incoming request
		reactor.listenTCP(self._port, site, self._connections,
		                  self._listenAddress)

		# start processing
		Logging.info("Start listening")
		log.addObserver(logSC3)

		return reactor.run()


	#---------------------------------------------------------------------------
	def _loadInventory(self):
		Logging.notice("Loading inventory")
		dbr = DataModel.DatabaseReader(self.database())
		self._inv = DataModel.Inventory()

		# Load networks and stations
		staCount = 0
		for i in xrange(dbr.loadNetworks(self._inv)):
			staCount += dbr.load(self._inv.network(i));
		Logging.debug("Loaded %i stations from %i networks" % (
		              staCount, self._inv.networkCount()))

		# Load sensors, skip calibrations (not needed by StationXML exporter)
		Logging.debug("Loaded %i sensors" % dbr.loadSensors(self._inv))

		# Load datalogger and its decimations, skip calibrations (not needed by
		# StationXML exporter)
		deciCount = 0
		for i in xrange(dbr.loadDataloggers(self._inv)):
			deciCount += dbr.loadDecimations(self._inv.datalogger(i))
		Logging.debug("Loaded %i decimations from %i dataloggers" % (
		              deciCount, self._inv.dataloggerCount()))

		# Load responses
		resPAZCount = dbr.loadResponsePAZs(self._inv)
		resFIRCount = dbr.loadResponseFIRs(self._inv)
		resPolCount = dbr.loadResponsePolynomials(self._inv)
		resCount = resPAZCount + resFIRCount + resPolCount
		Logging.debug("Loaded %i responses (PAZ: %i, FIR: %i, Poly: %i)" % (
		              resCount, resPAZCount, resFIRCount, resPolCount))
		Logging.info("Inventory loaded")

app = WSIris()
sys.exit(app())
