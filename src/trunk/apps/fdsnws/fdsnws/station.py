################################################################################
# Copyright (C) 2013 by gempa GmbH
#
# FDSNStation -- Implements the fdsnws-station Web service, see
#   http://www.fdsn.org/webservices/
#
# Feature notes:
#   - 'updatedafter' request parameter not implemented: The last modification
#     time in SeisComP is tracked on the object level. If a child of an object
#     is updated the update time is not propagated to all parents. In order to
#     check if a station was updated all children must be evaluated recursively.
#     This operation would be much to expensive.
#   - 'includeavailability' request parameter not implemented
#   - additional request parameters:
#     - format:          [fdsnxml, stationxml, sc3ml], default: fdsnxml
#     - formatted:       boolean, default: false
#
# Author:  Stephan Herrnkind
# Email:   herrnkind@gempa.de
################################################################################

from twisted.internet.threads import deferToThread
from twisted.web import http, resource, server

from seiscomp3 import DataModel, Logging
from seiscomp3.Client import Application
from seiscomp3.Core import Time, ValueException
from seiscomp3.IO import Exporter

from http import HTTP, NoResource
from request import RequestOptions
import utils


################################################################################
class _StationRequestOptions(RequestOptions):

	Exporters = { "fdsnxml"   : "fdsnxml",
	              "stationxml": "staxml",
	              "sc3ml"     : "trunk" }

	#---------------------------------------------------------------------------
	def __init__(self, args):
		RequestOptions.__init__(self, args)
		self.service = "fdsnws-station"

		self.includeSta = True
		self.includeCha = False
		self.includeRes = False

		self.restricted   = False
		self.availability = False
		self.updatedAfter = None

		# non standard parameters
		self.output     = "fdsnxml"
		self.formatted  = False

	#---------------------------------------------------------------------------
	def parse(self):
		self.parseNoData()
		self.parseChannel()
		self.parseTime(True)
		self.parseGeo()

		# level: [network, station, channel, response]
		if "level" in self._args:
			value = self._args["level"][0].lower()
			if value ==  "network" or value == "net":
				self.includeSta = False
			elif value == "channel" or value == "cha" or value == "chan":
				self.includeCha = True
			elif value == "response" or value == "res" or value == "resp":
				self.includeCha = True
				self.includeRes = True
			elif value != "station" and value != "sta":
				raise ValueError, "Invalid value in parameter: level"

		# includeRestricted (optional)
		if "includerestricted" in self._args:
			self.restricted = self.parseBoolean("includerestricted")
		# includeAvailability (optional), currently not supported
		if "includeavailability" in self._args:
			self.availability = self.parseBoolean("includeavailability")
		# updatedAfter (optional), currently not supported
		if "updatedafter" in self._args:
			self.updatedAfter = self.parseTimeStr("updatedafter")

		# output exporter
		if "format" in self._args:
			self.output = self._args["format"][0].lower()
			if not self.output in self.Exporters:
				raise ValueError, "Invalid value in parameter: format"

		# format XML
		if "formatted" in self._args:
			self.formatted = self.parseBoolean("formatted")


################################################################################
class FDSNStation(resource.Resource):
	isLeaf = True

	#---------------------------------------------------------------------------
	def __init__(self):
		resource.Resource.__init__(self)

		# additional object count dependent on detail level
		inv = Application.Instance()._inv
		self._resLevelCount = inv.responsePAZCount() + inv.responseFIRCount() \
		                      + inv.responsePolynomialCount()

		for i in xrange(inv.dataloggerCount()):
			self._resLevelCount += inv.datalogger(i).decimationCount()

	#---------------------------------------------------------------------------
	def render_GET(self, req):
		# Parse and validate GET parameters
		ro = _StationRequestOptions(req.args)
		try:
			ro.parse()
		except ValueError, e:
			Logging.warning(str(e))
			return HTTP.renderErrorPage(req, http.BAD_REQUEST, str(e), ro)

		if ro.availability:
			msg = "Including of availability information not supported"
			return HTTP.renderErrorPage(req, http.SERVICE_UNAVAILABLE, msg, ro)

		if ro.updatedAfter:
			msg = "Filtering based on update time not supported"
			return HTTP.renderErrorPage(req, http.SERVICE_UNAVAILABLE, msg, ro)

		# Exporter
		exp = Exporter.Create(ro.Exporters[ro.output])
		if exp:
			exp.setFormattedOutput(ro.formatted)
		else:
			msg = "Output format '%s' no available. Export module '%s' could " \
			      "not be loaded." % (ro.output, ro.Exporters[ro.output])
			return HTTP.renderErrorPage(req, http.SERVICE_UNAVAILABLE, msg, ro)

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

		DataModel.PublicObject.SetRegistrationEnabled(False)
		maxObj = Application.Instance()._queryObjects
		staCount, locCount, chaCount, objCount = 0, 0, 0, 0

		DataModel.PublicObject.SetRegistrationEnabled(False)
		inv = Application.Instance()._inv
		newInv = DataModel.Inventory()
		filterChannel = ro.channel and (ro.channel.loc or ro.channel.cha)
		dataloggers, sensors = set(), set()

		# iterate over inventory networks
		for net in utils.networkIter(inv, ro):
			if not ro.restricted and net.restricted(): continue
			newNet = DataModel.Network(net)

			# iterate over inventory stations of current network
			for sta in utils.stationIter(net, ro, matchGeo=True):
				if not ro.restricted and sta.restricted(): continue
				if not HTTP.checkObjects(req, objCount, maxObj): return False
				if ro.includeCha:
					numCha, numLoc, d, s = self._processStation(newNet, sta, ro)
					if numCha > 0:
						locCount += numLoc
						chaCount += numCha
						objCount += numLoc + numCha
						if not HTTP.checkObjects(req, objCount, maxObj):
							return False
						dataloggers |= d
						sensors |= s
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

		# Return 204 if no matching inventory was found
		if newInv.networkCount() == 0:
			utils.writeTS(req, HTTP.renderErrorPage(req, http.NO_CONTENT,
			              "No matching inventory found", ro))
			return False

		# Copy references (dataloggers, responses, sensors)
		decCount, resCount = 0, 0
		if ro.includeCha:
			decCount = self._copyReferences(newInv, req, objCount, inv, ro,
			           dataloggers, sensors)
			if decCount is None:
				return False
			else:
				resCount = newInv.responsePAZCount() + \
				           newInv.responseFIRCount() + \
				           newInv.responsePolynomialCount()
				objCount += resCount + decCount + newInv.dataloggerCount() + \
				            newInv.sensorCount()

		req.setHeader("Content-Type", "application/xml")
		sink = utils.Sink(req)
		if not exp.write(sink, newInv):
			return False

		Logging.notice("%s: returned %iNet, %iSta, %iLoc, %iCha, " \
		               "%iDL, %iDec, %iSen, %iRes (total objects/bytes: " \
		               "%i/%i) " % (ro.service, newInv.networkCount(), staCount,
		               locCount, chaCount, newInv.dataloggerCount(), decCount,
		               newInv.sensorCount(), resCount, objCount, sink.written))
		utils.accessLog(req, ro, http.OK, sink.written, None)
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
			if not ro.channel.cha and not ro.time:
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
		dataloggers, sensors = set(), set()
		newSta = DataModel.Station(sta)
		for loc in utils.locationIter(sta, ro):
			newLoc = DataModel.SensorLocation(loc)
			for stream in utils.streamIter(loc, ro):
				newLoc.add(DataModel.Stream(stream))
				dataloggers.add(stream.datalogger())
				sensors.add(stream.sensor())

			if newLoc.streamCount() > 0:
				newSta.add(newLoc)
				chaCount += newLoc.streamCount()

		if newSta.sensorLocationCount() > 0:
			newNet.add(newSta)
			return chaCount, newSta.sensorLocationCount(), dataloggers, sensors

		return 0, 0, [], []


	#---------------------------------------------------------------------------
	# Copy references (data loggers, sensors, responses) depended on request
	# options
	@staticmethod
	def _copyReferences(newInv, req, objCount, inv, ro, dataloggers, sensors):

		respPAZ, respFIR, respPoly = set(), set(), set()
		maxObj = Application.Instance()._queryObjects
		decCount = 0

		# datalogger
		for i in xrange(inv.dataloggerCount()):
			logger = inv.datalogger(i)
			if logger.publicID() not in dataloggers:
				continue
			newLogger = DataModel.Datalogger(logger)
			newInv.add(newLogger)
			# decimations are only needed for responses
			if ro.includeRes:
				for i in xrange(logger.decimationCount()):
					decimation = logger.decimation(i)
					newLogger.add(DataModel.Decimation(logger.decimation(i)))

					# collect response ids
					filterStr = ""
					try: filterStr = decimation.analogueFilterChain().content()
					except ValueException: pass
					try: filterStr += decimation.digitalFilterChain().content()
					except ValueException: pass
					for resp in filterStr.split():
						if resp.startswith("ResponsePAZ"):
							respPAZ.add(resp)
						elif resp.startswith("ResponseFIR"):
							respFIR.add(resp)
						elif resp.startswith("ResponsePolynomial"):
							respPoly.add(resp)
				decCount += newLogger.decimationCount()

		objCount += newInv.dataloggerCount() + decCount
		resCount = len(respPAZ) + len(respFIR) + len(respPoly)
		if not HTTP.checkObjects(req, objCount + resCount, maxObj):
			return None

		# sensor
		for i in xrange(inv.sensorCount()):
			sensor = inv.sensor(i)
			if sensor.publicID() not in sensors:
				continue
			newSensor = DataModel.Sensor(sensor)
			newInv.add(newSensor)
			resp = newSensor.response()
			if resp:
				if ro.includeRes:
					if resp.startswith("ResponsePAZ"):
						respPAZ.add(resp)
					elif resp.startswith("ResponseFIR"):
						respFIR.add(resp)
					elif resp.startswith("ResponsePolynomial"):
						respPoly.add(resp)
				else:
					# no responses: remove response reference to avoid missing
					# response warning of exporter
					newSensor.setResponse("")

		objCount += newInv.sensorCount()
		resCount = len(respPAZ) + len(respFIR) + len(respPoly)
		if not HTTP.checkObjects(req, objCount + resCount, maxObj):
			return None

		# responses
		if ro.includeRes:
			for i in xrange(inv.responsePAZCount()):
				resp = inv.responsePAZ(i)
				if resp.publicID() in respPAZ:
					newInv.add(DataModel.ResponsePAZ(resp))
			for i in xrange(inv.responseFIRCount()):
				resp = inv.responseFIR(i)
				if resp.publicID() in respFIR:
					newInv.add(DataModel.ResponseFIR(resp))
			for i in xrange(inv.responsePolynomialCount()):
				resp = inv.responsePolynomial(i)
				if resp.publicID() in respPoly:
					newInv.add(DataModel.ResponsePolynomial(resp))

		return decCount
