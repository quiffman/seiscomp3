################################################################################
# Copyright (C) 2012 by gempa GmbH
#
# WSStation -- Implements the IRIS ws-station Web service, see
#   http://www.iris.edu/ws/station/
#
# Feature notes:
#   - 'updatedafter' request parameter not implemented: The last modification
#     time in SeisComP is tracked on the object level. If a child of an object
#     is updated the update time is not propagated to all parents. In order to
#     check if a station was updated all children must be evaluated recursively.
#     This operation would be much to expensive.
#   - additional request parameters:
#     - output:          [stationxml, sc3ml], default: stationxml
#     - formatted:       boolean, default: false
#
# Author:  Stephan Herrnkind
# Email:   herrnkind@gempa.de
################################################################################

from twisted.internet.threads import deferToThread
from twisted.web import http, resource, server

from seiscomp3 import DataModel, Logging
from seiscomp3.Client import Application
from seiscomp3.Core import Time
from seiscomp3.IO import Exporter

from http import HTTP, NoResource
from request import RequestOptions
import utils


################################################################################
class _StationRequestOptions(RequestOptions):

	Exporters = { "stationxml": "staxml",
	              "sc3ml"     : "trunk" }

	#---------------------------------------------------------------------------
	def __init__(self, args):
		RequestOptions.__init__(self, args)
		self.updatedAfter = None
		self.includeSta = True
		self.includeCha = False
		self.includeRes = False

		# non standard parameters
		self.output     = "stationxml"
		self.formatted  = False

	#---------------------------------------------------------------------------
	def parse(self):
		self.parseChannelGlobs()
		self.parseTime(True)
		self.parseGeo()

		# updatedAfter (optional), currently not supported
		if "updatedafter" in self._args:
			self.updatedAfter = self.parseTimeStr("updatedafter")

		# level: [net, sta, chan, resp]
		if "level" in self._args:
			value = self._args["level"].lower()
			if value ==  "net":
				self.includeSta = False
			elif value == "chan":
				self.includeCha = True
			elif value == "resp":
				self.includeCha = True
				self.includeRes = True
			elif value != "sta":
				raise ValueError, "Invalid value in parameter: level"

		# output exporter
		if "output" in self._args:
			self.output = self._args["output"].lower()
			if not self.output in self.Exporters:
				raise ValueError, "Invalid value in parameter: output"

		# format XML
		if "formatted" in self._args:
			self.formatted = self.parseBoolean("formatted")


################################################################################
class WSStation(resource.Resource):
	isLeaf = True

	#---------------------------------------------------------------------------
	def __init__(self):
		resource.Resource.__init__(self)

		# additional object count dependent on detail level
		inv = Application.Instance()._inv
		self._chaLevelCount = inv.dataloggerCount() + inv.sensorCount()
		self._resLevelCount = inv.responsePAZCount() + inv.responseFIRCount() \
		                      + inv.responsePolynomialCount()

		for i in xrange(inv.dataloggerCount()):
			self._resLevelCount += inv.datalogger(i).decimationCount()

	#---------------------------------------------------------------------------
	def render_GET(self, req):
		# Parse and validate GET parameters
		try:
			ro = _StationRequestOptions(req.args)
			ro.parse()
		except ValueError, e:
			Logging.warning(str(e))
			return HTTP.renderErrorPage(req, http.BAD_REQUEST, str(e))

		# Exporter
		exp = Exporter.Create(ro.Exporters[ro.output])
		if exp:
			exp.setFormattedOutput(ro.formatted)
		else:
			msg = "Output format '%s' no available. Export module '%s' could " \
			      "not be loaded." % (ro.output, ro.Exporters[ro.output])
			return HTTP.renderErrorPage(req, http.SERVICE_UNAVAILABLE, msg)

		# Process request in separate thread
		d = deferToThread(self._processRequest, req, ro, exp)
		d.addCallback(utils.onRequestServed, req)
		d.addErrback(utils.onRequestError, req)

		# The request is handled by the deferred object
		return server.NOT_DONE_YET


	#---------------------------------------------------------------------------
	def _processRequest(self, req, ro, exp):
		if req._disconnected:
			return False

		maxObj = Application.Instance()._queryObjects
		staCount, chaCount, locCount, objCount = 0, 0, 0, 0

		DataModel.PublicObject.SetRegistrationEnabled(False)
		inv = Application.Instance()._inv
		newInv = DataModel.Inventory()
		filterChannel = ro.channel and (ro.channel.loc or ro.channel.cha)

		# iterate over inventory networks
		for net in utils.networkIter(inv, ro):
			if not utils.checkObjects(req, objCount, maxObj): return False
			newNet = DataModel.Network(net)

			# iterate over inventory stations of current network
			for sta in utils.stationIter(net, ro):
				if ro.includeCha:
					numCha, numLoc = self._processStation(newNet, sta, ro)
					if numCha > 0:
						chaCount += numCha
						locCount += numLoc
						objCount += numCha + numLoc
						if not utils.checkObjects(req, objCount, maxObj):
							return False
				elif self._matchStation(sta, ro):
					if ro.includeSta:
						newNet.add(DataModel.Station(sta))
					else:
						# no station output requested: one matching station is
						# sufficient to include the network
						newInv.add(newNet)
						objCount += 1
						break

			if newNet.stationCount() > 0:
				newInv.add(newNet)
				staCount += newNet.stationCount()
				objCount += staCount + 1

		# Return 404 if no matching station was found
		if newInv.networkCount() == 0:
			utils.writeTS(req, HTTP.renderErrorPage(req, http.NOT_FOUND,
			                                        "No stations found"))
			return False

		# Copy references (if object limit allows to do so)
		if ro.includeCha:
			objCount += self._chaLevelCount
		if ro.includeRes:
			objCount += self._resLevelCount
		if not utils.checkObjects(req, objCount, maxObj): return False
		self._copyReferences(newInv, inv, ro)

		req.setHeader("Content-Type", "text/xml")
		if not exp.write(utils.Sink(req), newInv):
			return False

		Logging.notice("WS-Station: Returned %i networks, %i stations and %i " \
		               "streams (total objects: %i)" % (newInv.networkCount(),
		               staCount, chaCount, objCount))
		return True


	#---------------------------------------------------------------------------
	# Checks if at least one location and channel combination matches the
	# request options
	@staticmethod
	def _matchStation(sta, ro):
		# No filter: return true immediately
		if not ro.channel or (not ro.channel.loc and not ro.channel.cha):
			return True

		for loc in utils.locationIter(sta, ro):
			if not ro.channel.cha:
				return True

			for stream in utils.streamIter(loc, ro):
				return True

		return False


	#---------------------------------------------------------------------------
	# Adds a deep copy of the specified station to the new network if the
	# location and channel combination matches the request options (if any)
	@staticmethod
	def _processStation(newNet, sta, ro):
		chaCount = 0
		newSta = DataModel.Station(sta)
		for loc in utils.locationIter(sta, ro):
			newLoc = DataModel.SensorLocation(loc)
			for stream in utils.streamIter(loc, ro):
				newLoc.add(DataModel.Stream(stream))

			if newLoc.streamCount() > 0:
				newSta.add(newLoc)
				chaCount += newLoc.streamCount()

		if newSta.sensorLocationCount() > 0:
			newNet.add(newSta)
			return chaCount, newSta.sensorLocationCount()

		return 0, 0


	#---------------------------------------------------------------------------
	# Copy references (data loggers, sensors, responses) depended on request
	# options
	@staticmethod
	def _copyReferences(newInv, inv, ro):
		if not ro.includeCha:
			return

		# data logger
		for i in xrange(inv.dataloggerCount()):
			log = inv.datalogger(i)
			newLog = DataModel.Datalogger(log)
			# decimations are only needed for responses
			if ro.includeRes:
				for i in xrange(log.decimationCount()):
					newLog.add(DataModel.Decimation(log.decimation(i)))
			newInv.add(newLog)

		# sensor
		for i in xrange(inv.sensorCount()):
			newSen = DataModel.Sensor(inv.sensor(i))
			# no responses: remove response reference to avoid missing response
			# warning of exporter
			if not ro.includeRes:
				newSen.setResponse("")
			newInv.add(newSen)

		# responses
		if ro.includeRes:
			for i in xrange(inv.responsePAZCount()):
				newInv.add(DataModel.ResponsePAZ(inv.responsePAZ(i)))
			for i in xrange(inv.responseFIRCount()):
				newInv.add(DataModel.ResponseFIR(inv.responseFIR(i)))
			for i in xrange(inv.responsePolynomialCount()):
				newInv.add(DataModel.ResponsePolynomial(
				           inv.responsePolynomial(i)))
