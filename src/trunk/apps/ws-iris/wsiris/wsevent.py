################################################################################
# Copyright (C) 2012 by gempa GmbH
#
# WSEvent -- Implements the IRIS ws-event Web service, see
#   http://www.iris.edu/ws/event/
#
# Feature notes:
#   - SeisComP does not distinguish between catalogs and contributors, but
#     supports agencyIDs. Hence, if specified, the value of the 'catalog' or
#     'contributor' parameter is mapped to the agencyID. If both parameters are
#     supplied, the value of the 'contributor' is used.
#   - origin and magnitude filter parameters are always applied to
#     preferred origin resp. preferred magnitude
#   - additional request parameters:
#     - output:          [qml, qml-rt, sc3ml], default: qml
#     - includecomments: boolean, default: true
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
class _EventRequestOptions(RequestOptions):

	Exporters = { "qml"    : "qml1.2",
	              "qml-rt" : "qml1.2rt",
	              "sc3ml"  : "trunk" }

	#---------------------------------------------------------------------------
	class Depth:
		def __init__(self):
			self.min = None
			self.max = None

	#---------------------------------------------------------------------------
	class Magnitude:
		def __init__(self):
			self.min  = None
			self.max  = None
			self.type = None


	#---------------------------------------------------------------------------
	def __init__(self, args):
		RequestOptions.__init__(self, args)

		self.depth        = None
		self.mag          = None
		self.agencyID     = None      # read from catalog/contributor parameter
		self.limit        = None      # event limit, if defined: min 10
		self.orderBy      = None      # [time, magnitude]
		self.updatedAfter = None      # not supported

		self.eventID      = None      # specific event filter may not be
		                              # combined with above filter criteria

		self.magnitudes   = True
		self.arrivals     = False
		self.prefOnly     = True

		# non standard parameters
		self.output       = "qml"
		self.comments     = True
		self.formatted    = False


	#---------------------------------------------------------------------------
	def parse(self):
		# geo filter (bounding box or bounding circle)
		self.parseGeo()

		# depth
		d = self.Depth()
		if "mindepth" in self._args:
			d.min = self.parseFloat("mindepth")
		if "maxdepth" in self._args:
			d.max = self.parseFloat("maxdepth", d.min) if d.min else \
			        self.parseFloat("maxdepth")
		if d.min or d.max:
			self.depth = d

		# time
		self.parseTime()

		# magnitude
		m = self.Magnitude()
		if "minmag" in self._args:
			m.min = self.parseFloat("minmag")
		if "maxmag" in self._args:
			m.max = self.parseFloat("maxmag", m.min) if m.min else \
			        self.parseFloat("maxmag")
		if "magtype" in self._args:
			m.type = self._args["magtype"]
		if m.min or m.max or m.type:
			self.mag = m

		# agencyID (catalog/contributor)
		if "contributor" in self._args:
			self.agencyID = self._args["contributor"]
		elif "catalog" in self._args:
			self.agencyID = self._args["catalog"]

		# limit, orderBy, updatedAfter
		if "limit" in self._args:
			self.limit = int(self.parseFloat("limit", 10))
		if "orderby" in self._args:
			value = self._args["orderby"].lower()
			if value in ("time", "magnitude"):
				self.orderBy = value
			else:
				raise ValueError, "Invalid value in parameter: orderby"
		if "updatedafter" in self._args:
			self.updatedAfter = self.parseTimeStr("updatedafter")

		requiredParams = self.geo or self.depth or self.time or self.mag
		optionalParams = self.agencyID or self.limit or self.orderBy or \
		                 self.updatedAfter
		if "eventid" in self._args:
			# eventID, MUST NOT be combined with above parameters
			if requiredParams or optionalParams:
				raise ValueError, "Invalid mixture of parameters. The " \
				      "parameter 'eventid' may only be combined with: " \
				      "includeallmagnitudes, preferredonly and includearrivals"
			self.eventID = self._args["eventid"]
		elif requiredParams:
			# set defaults for none eventID query
			if not self.orderBy:
				self.orderBy = "time"
		else:
			raise ValueError, "Insufficient search parameters"

		# output components
		if "includeallmagnitudes" in self._args:
			self.magnitudes = self.parseBoolean("includeallmagnitudes")
		if "includearrivals" in self._args:
			self.arrivals = self.parseBoolean("includearrivals")
		if "preferredonly" in self._args:
			self.prefOnly = self.parseBoolean("preferredonly")
		if "includecomments" in self._args:
			self.comments = self.parseBoolean("includecomments")

		# output exporter
		if "output" in self._args:
			self.output = self._args["output"].lower()
			if not self.output in self.Exporters:
				raise ValueError, "Invalid value in parameter: output"

		# format XML
		if "formatted" in self._args:
			self.formatted = self.parseBoolean("formatted")


################################################################################
class WSEvent(resource.Resource):
	isLeaf = True

	#---------------------------------------------------------------------------
	def render_GET(self, req):
		# Parse and validate GET parameters
		try:
			ro = _EventRequestOptions(req.args)
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

		# Create database query
		dbq = DataModel.DatabaseQuery(Application.Instance().database())
		if dbq.hasError():
			msg = "Could not connect to database: %s" % dbq.errorMsg()
			return HTTP.renderErrorPage(req, http.SERVICE_UNAVAILABLE, msg)

		# Process request in separate thread
		d = deferToThread(self._processRequest, req, ro, dbq, exp)
		d.addCallback(utils.onRequestServed, req)
		d.addErrback(utils.onRequestError, req)

		# The request is handled by the deferred object
		return server.NOT_DONE_YET


	#---------------------------------------------------------------------------
	def _processRequest(self, req, ro, dbq, exp):
		if req._disconnected:
			return False

		maxObj = Application.Instance()._queryObjects

		# query event(s)
		ep = DataModel.EventParameters()
		if ro.eventID:
			event = dbq.getEventByPublicID(ro.eventID)
			event = DataModel.Event.Cast(event)
			if event:
				ep.add(event)
			else:
				msg = "No event with id '%s' found" % ro.eventID
				utils.writeTS(req, HTTP.renderErrorPage(req, http.NOT_FOUND, msg))
				return False
		else:
			self._findEvents(ep, ro, dbq)
			if ep.eventCount() == 0:
				msg = "No matching events found"
				utils.writeTS(req, HTTP.renderErrorPage(req, http.NOT_FOUND, msg))
				return False

		objCount = ep.eventCount()
		Logging.debug("Events found: %i" % objCount)
		if not utils.checkObjects(req, objCount, maxObj): return False

		# add related information
		for iEvent in xrange(ep.eventCount()):
			e = ep.event(iEvent)
			eID = e.publicID()

			# event descriptions
			typeInfo = DataModel.EventDescription.TypeInfo()
			for obj in dbq.getObjects(eID, typeInfo):
				e.add(DataModel.EventDescription.Cast(obj))
			objCount += e.eventDescriptionCount()

			# comments
			if ro.comments:
				self._loadComments(e, dbq)
				objCount += e.commentCount()

			# origin references: either preferred only or all
			if ro.prefOnly:
				e.add(DataModel.OriginReference(e.preferredOriginID()))
			else:
				typeInfo = DataModel.OriginReference.TypeInfo()
				for obj in dbq.getObjects(eID, typeInfo):
					e.add(DataModel.OriginReference.Cast(obj))
			objCount += e.originReferenceCount()
			if not utils.checkObjects(req, objCount, maxObj): return False

			# origins
			typeInfo = DataModel.Origin.TypeInfo()
			for iORef in xrange(e.originReferenceCount()):
				oID = e.originReference(iORef).originID()
				obj = dbq.getObject(typeInfo, oID)
				o = DataModel.Origin.Cast(obj)
				if not o:
					continue

				ep.add(o)
				objCount += 1

				# comments
				if ro.comments:
					self._loadComments(o, dbq)
					objCount += o.commentCount()

				# arrivals
				if ro.arrivals:
					typeInfo = DataModel.Arrival.TypeInfo()
					for obj in dbq.getObjects(oID, typeInfo):
						o.add(DataModel.Arrival.Cast(obj))
					objCount += o.arrivalCount()

				# magnitudes
				if ro.magnitudes:
					typeInfo = DataModel.Magnitude.TypeInfo()
					for obj in dbq.getObjects(oID, typeInfo):
						o.add(DataModel.Magnitude.Cast(obj))
					objCount += o.magnitudeCount()
					if ro.comments:
						for iMag in xrange(o.magnitudeCount()):
							mag = o.magnitude(iMag)
							self._loadComments(mag, dbq)
							objCount += mag.commentCount()

				if not utils.checkObjects(req, objCount, maxObj): return False

		req.setHeader("Content-Type", "text/xml")
		if not exp.write(utils.Sink(req), ep):
			return False

		Logging.notice("WS-Event: Returned %i events and %i origins (total " \
		               "objects: %i)" % (ep.eventCount(), ep.originCount(),
		                                 objCount))
		return True


	#---------------------------------------------------------------------------
	def _findEvents(self, ep, ro, dbq):
		db = Application.Instance().database()
		def _T(name):
			return db.convertColumnName(name)
		def _time(time):
			return db.timeToString(time)

		reqMag = ro.mag or ro.orderBy == "magnitude"
		reqDist = ro.geo and ro.geo.bCircle
		colPID = _T("publicID")
		colTime = _T("time_value")

		bBox = None
		if ro.geo:
			colLat, colLon = _T("latitude_value"), _T("longitude_value")
			if ro.geo.bBox:
				bBox = ro.geo.bBox
			else:
				bBox = ro.geo.bCircle.calculateBBox()

		# SELECT --------------------------------
		q = "SELECT pe.%s, e.*" % colPID
		if reqDist: # Great circle distance calculated by Haversine formula
			c = ro.geo.bCircle
			q += ", DEGREES(ACOS(" \
			     "COS(RADIANS(o.%s)) * COS(RADIANS(%f)) * " \
			     "COS(RADIANS(o.%s) - RADIANS(%f)) + SIN(RADIANS(o.%s)) * " \
			     "SIN(RADIANS(%f)))) AS distance" % (
			     colLat, c.lat, colLon, c.lon, colLat, c.lat)

		# FROM ----------------------------------
		q += " FROM Event AS e, PublicObject AS pe" \
		     ", Origin AS o, PublicObject AS po"
		if reqMag:
			q += ", Magnitude AS m, PublicObject AS pm"

		# WHERE ---------------------------------
		q += " WHERE e._oid = pe._oid"

		# event information filter
		if ro.agencyID:
			q += " AND e.%s AND e.%s LIKE '%%%s%%'" % (_T("creationinfo_used"),
			     _T("creationinfo_agencyid"), ro.agencyID)

		# origin information filter
		q += " AND o._oid = po._oid AND po.%s = e.%s" % (
		       colPID, _T("preferredOriginID"))

		# time
		if ro.time:
			colTimeMS = _T("time_value_ms")
			if ro.time.start:
				q += " AND o.%s >= '%s'" % (colTime, _time(ro.time.start))
				ms = ro.time.start.microseconds()
				if ms > 0: q += " AND o.%s >= %i" % (colTimeMS, ms)
			if ro.time.end:
				q += " AND o.%s c= '%s'" % (colTime, _time(ro.time.end))
				ms = ro.time.end.microseconds()
				if ms > 0: q += " AND o.%s <= %i" % (colTimeMS, ms)

		# bounding box
		if bBox:
			if bBox.minLat:
				q += " AND o.%s >= %f" % (colLat, bBox.minLat)
			if bBox.maxLat:
				q += " AND o.%s <= %f" % (colLat, bBox.maxLat)
			if bBox.dateLineCrossing():
				q += " AND (o.%s >= %f OR o.%s <= %f)" % (
				     colLon, bBox.minLon, colLon, bBox.maxLon)
			else:
				if bBox.minLon:
					q += " AND o.%s >= %f" % (colLon, bBox.minLon)
				if bBox.maxLon:
					q += " AND o.%s <= %f" % (colLon, bBox.maxLon)

		# depth
		if ro.depth:
			q += " AND o.%s" % _T("depth_used")
			colDepth = _T("depth_value")
			if ro.depth.min:
				q += " AND o.%s >= %f" % (colDepth, ro.depth.min)
			if ro.depth.max:
				q += " AND o.%s <= %f" % (colDepth, ro.depth.max)

		# updated after
		if ro.updatedAfter:
			timeStr = _time(ro.updatedAfter)
			ms = ro.updatedAfter.microseconds()
			colCTime = _T("creationinfo_creationtime")
			colMTime = _T("creationinfo_modificationtime")
			if ms > 0:
				colCTimeMS = _T("creationinfo_creationtime_ms")
				colMTimeMS = _T("creationinfo_modificationtime_ms")
				tFilter = "(o.%s >= '%s' AND o.%s > %i)"
			else:
				tFilter = "o.%s > '%s'"

			q += " AND ("
			if ms > 0: q += tFilter % (colCTime, timeStr, colCTimeMS, ms)
			else:      q += tFilter % (colCTime, timeStr)
			q += " OR "
			if ms > 0: q += tFilter % (colMTime, timeStr, colMTimeMS, ms)
			else:      q += tFilter % (colMTime, timeStr)
			q += ")"

		# magnitude information filter
		if reqMag:
			q += " AND m._oid = pm._oid AND pm.%s = e.%s" % (
			     colPID, _T("preferredMagnitudeID"))

			colMag = _T("magnitude_value")
			if ro.mag:
				if ro.mag.min or ro.mag.max:
					if ro.mag.min:
						q += " AND m.%s >= %f" % (colMag, ro.mag.min)
					if ro.mag.max:
						q += " AND m.%s <= %f" % (colMag, ro.mag.max)

				if ro.mag.type:
					q += " AND m.%s = '%s'" % (_T("type"), ro.mag.type)

		# ORDER BY ------------------------------
		q += " ORDER BY "
		if ro.orderBy == "time":
			q += "o.%s" % colTime
		else:
			q += "m.%s" % colMag
		q += " DESC"

		# SUBQUERY distance (optional) ----------
		if reqDist:
			q = "SELECT * FROM (%s) AS subquery" % q
			c = ro.geo.bCircle
			q += " WHERE distance <= %f" % c.maxRad
			if c.minRad:
				q += " AND distance >= %f" % c.minRad

		# LIMIT ---------------------------------
		if ro.limit:
			q += " LIMIT %i" % ro.limit

		Logging.debug("Event query: %s" % q)

		for e in dbq.getObjectIterator(q, DataModel.Event.TypeInfo()):
			ep.add(DataModel.Event.Cast(e))


	#---------------------------------------------------------------------------
	@staticmethod
	def _loadComments(container, dbq):
		typeInfo = DataModel.Comment.TypeInfo()
		for obj in dbq.getObjects(container.publicID(), typeInfo):
			container.add(DataModel.Comment.Cast(obj))

