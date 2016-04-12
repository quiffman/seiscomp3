################################################################################
# Copyright (C) 2013 by gempa GmbH
#
# FDSNEvent -- Implements the fdsnws-event Web service, see
#   http://www.fdsn.org/webservices/
#
# Feature notes:
#   - SeisComP does not distinguish between catalogs and contributors, but
#     supports agencyIDs. Hence, if specified, the value of the 'contributor'
#     parameter is mapped to the agencyID.
#   - origin and magnitude filter parameters are always applied to
#     preferred origin resp. preferred magnitude
#   - additional request parameters:
#     - format:          [qml, qml-rt, sc3ml], default: qml
#     - includepicks:    boolean, default: false
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


DBMaxUInt = 18446744073709551615 # 2^64 - 1

################################################################################
class _EventRequestOptions(RequestOptions):

	Exporters = { "qml"    : "qml1.2",
	              "qml-rt" : "qml1.2rt",
	              "sc3ml"  : "trunk",
	              "csv"    : "csv" }

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
		self.service = "fdsnws-event"

		self.depth        = None
		self.mag          = None

		self.allOrigins   = False
		self.allMags      = False
		self.arrivals     = False

		self.limit        = None   # event limit, if defined: min 1
		self.offset       = None   # start at specific event count position,
		                           # the spec uses a weird offset definition
		                           # where an offset of '1' returns the first
		                           # element, not the second one
		self.orderBy      = None   # [time, time-asc, magnitude, magnitude-asc]
		self.catalogs     = []     # not supported
		self.contributors = []     # mapped to agency id
		self.updatedAfter = None

		self.eventIDs     = []     # specific event filter may not be
		                           # combined with above filter criteria


		# non standard parameters
		self.output       = "qml"
		self.comments     = True
		self.formatted    = False
		self.picks        = False


	#---------------------------------------------------------------------------
	def parse(self):
		self.parseNoData()
		self.parseTime()
		self.parseGeo()

		# depth
		d = self.Depth()
		if "mindepth" in self._args:
			d.min = self.parseFloat("mindepth")
		if "maxdepth" in self._args:
			d.max = self.parseFloat("maxdepth")
		if d.min is not None and d.max is not None and d.min > d.max:
			raise ValueError, "Mindepth exceeds maxdepth"
		if d.min is not None or d.max:
			self.depth = d

		# magnitude
		m = self.Magnitude()
		if "minmag" in self._args:
			m.min = self.parseFloat("minmag")
		elif "minmagnitude" in self._args:
			m.min = self.parseFloat("minmagnitude")
		if "maxmag" in self._args:
			m.max = self.parseFloat("maxmag")
		elif "maxmagnitude" in self._args:
			m.max = self.parseFloat("maxmagnitude")
		if m.min is not None and m.max is not None and m.min > m.max:
			raise ValueError, "Minmagnitude exceeds maxmagnitude"
		if "magtype" in self._args:
			m.type = self._args["magtype"][0]
		elif "magnitudetype" in self._args:
			m.type = self._args["magnitudetype"][0]
		if m.min is not None or m.max is not None or m.type is not None:
			self.mag = m

		# output components
		if "includeallorigins" in self._args:
			self.allOrigins = self.parseBoolean("includeallorigins")
		if "includeallmagnitudes" in self._args:
			self.allMags = self.parseBoolean("includeallmagnitudes")
		if "includearrivals" in self._args:
			self.arrivals = self.parseBoolean("includearrivals")
		if "includepicks" in self._args:
			self.picks = self.parseBoolean("includepicks")
		if "includecomments" in self._args:
			self.comments = self.parseBoolean("includecomments")

		# limit, orderBy, updatedAfter
		if "limit" in self._args:
			self.limit = self.parseLong("limit", 1, DBMaxUInt)
		if "offset" in self._args:
			# the offset is decremented by one since spec uses a weird offset
			# where an offset of '1' should return the first element instead of
			# the second one
			self.offset = self.parseLong("offset", 1, DBMaxUInt) - 1
		if "orderby" in self._args:
			value = self._args["orderby"][0].lower()
			if value in ("time", "time-asc", "magnitude", "magnitude-asc"):
				self.orderBy = value
			else:
				raise ValueError, "Invalid value in parameter: orderby"
		if "updatedafter" in self._args:
			self.updatedAfter = self.parseTimeStr("updatedafter")

		# catalogs and contributors
		if "catalog" in self._args:
			self.catalogs = self._args["catalog"]
		if "contributor" in self._args:
			self.contributors = self._args["contributor"]

		# eventID(s)
		filterParams = self.time or self.geo or self.depth or self.mag or \
		               self.limit is not None or self.offset is not None or \
		               self.orderBy or self.catalogs or self.contributors or \
		               self.updatedAfter
		if "eventid" in self._args:
			# eventID, MUST NOT be combined with above parameters
			if filterParams:
				raise ValueError, "Invalid mixture of parameters. The " \
				      "parameter 'eventid' may only be combined with: " \
				      "includeallorigins, includeallmagnitudes, " \
				      "includearrival, and includecomments"
			self.eventIDs = self._args["eventid"]

		# format exporter
		if "format" in self._args:
			self.output = self._args["format"][0].lower()
			if not self.output in self.Exporters:
				raise ValueError, "Invalid value in parameter: format"

		# include comments
		if "comments" in self._args:
			self.comments = self.parseBoolean("comments")

		# format XML
		if "formatted" in self._args:
			self.formatted = self.parseBoolean("formatted")


################################################################################
class FDSNEvent(resource.Resource):
	isLeaf = True

	#---------------------------------------------------------------------------
	def render_GET(self, req):
		# Parse and validate GET parameters
		ro = _EventRequestOptions(req.args)
		try:
			ro.parse()
		except ValueError, e:
			Logging.warning(str(e))
			return HTTP.renderErrorPage(req, http.BAD_REQUEST, str(e), ro)

		# Catalog filter is not supported, any filter value will result in 204
		if ro.catalogs:
			msg = "No matching events found"
			return HTTP.renderErrorPage(req, http.NO_CONTENT, msg, ro)

		# Exporter
		exp = Exporter.Create(ro.Exporters[ro.output])
		if exp:
			exp.setFormattedOutput(ro.formatted)
		else:
			msg = "Output format '%s' no available. Export module '%s' could " \
			      "not be loaded." % (ro.output, ro.Exporters[ro.output])
			return HTTP.renderErrorPage(req, http.SERVICE_UNAVAILABLE, msg, ro)

		# Create database query
		dbq = DataModel.DatabaseQuery(Application.Instance().database())
		if dbq.hasError():
			msg = "Could not connect to database: %s" % dbq.errorMsg()
			return HTTP.renderErrorPage(req, http.SERVICE_UNAVAILABLE, msg, ro)

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

		DataModel.PublicObject.SetRegistrationEnabled(False)
		maxObj = Application.Instance()._queryObjects

		# query event(s)
		ep = DataModel.EventParameters()
		if ro.eventIDs:
			for eID in ro.eventIDs:
				event = dbq.getEventByPublicID(eID)
				event = DataModel.Event.Cast(event)
				if event:
					ep.add(event)
		else:
			self._findEvents(ep, ro, dbq)

		if ep.eventCount() == 0:
			msg = "No matching events found"
			utils.writeTS(req,
			              HTTP.renderErrorPage(req, http.NO_CONTENT, msg, ro))
			return False

		objCount = ep.eventCount()
		Logging.debug("events found: %i" % objCount)
		if not HTTP.checkObjects(req, objCount, maxObj): return False

		pickIDs = set()

		# add related information
		for iEvent in xrange(ep.eventCount()):
			e = ep.event(iEvent)
			eID = e.publicID()

			# eventDescriptions and comments
			objCount += dbq.loadEventDescriptions(e)
			if ro.comments:
				objCount += dbq.loadComments(e)
			if not HTTP.checkObjects(req, objCount, maxObj): return False

			# origin references: either all or preferred only
			dbIter = dbq.getObjects(e, DataModel.OriginReference.TypeInfo())
			for obj in dbIter:
				oRef = DataModel.OriginReference.Cast(obj)
				if oRef is None:
					continue
				if ro.allOrigins:
					e.add(oRef)
				elif oRef.originID() == e.preferredOriginID():
					e.add(oRef)
					dbIter.close()
			objCount += e.originReferenceCount()
			# TODO: load FocalMechanismReferences???
			if not HTTP.checkObjects(req, objCount, maxObj): return False

			# origins
			for iORef in xrange(e.originReferenceCount()):
				oID = e.originReference(iORef).originID()
				obj = dbq.getObject(DataModel.Origin.TypeInfo(), oID)
				o = DataModel.Origin.Cast(obj)
				if o is None:
					continue

				ep.add(o)
				objCount += 1

				# comments
				if ro.comments:
					objCount += dbq.loadComments(o)
				if not HTTP.checkObjects(req, objCount, maxObj): return False

				# magnitudes
				dbIter = dbq.getObjects(oID, DataModel.Magnitude.TypeInfo())
				for obj in dbIter:
					mag = DataModel.Magnitude.Cast(obj)
					if mag is None:
						continue
					if ro.allMags:
						o.add(mag)
					elif mag.publicID() == e.preferredMagnitudeID():
						o.add(mag)
						dbIter.close()
				objCount += o.magnitudeCount()
				if ro.comments:
					for iMag in xrange(o.magnitudeCount()):
						objCount += dbq.loadComments(o.magnitude(iMag))
				if not HTTP.checkObjects(req, objCount, maxObj): return False

				# arrivals
				if ro.arrivals:
					objCount += dbq.loadArrivals(o)

					# collect pick IDs if requested
					if ro.picks:
						for iArrival in xrange(o.arrivalCount()):
							pickIDs.add(o.arrival(iArrival).pickID())

				if not HTTP.checkObjects(req, objCount, maxObj): return False

		# picks
		if pickIDs:
			objCount += len(pickIDs)
			if not HTTP.checkObjects(req, objCount, maxObj): return False
			for pickID in pickIDs:
				obj = dbq.getObject(DataModel.Pick.TypeInfo(), pickID)
				pick = DataModel.Pick.Cast(obj)
				if pick is not None:
					ep.add(pick)


		if ro.output == "csv":
			req.setHeader("Content-Type", "text/plain")
		else:
			req.setHeader("Content-Type", "application/xml")
		sink = utils.Sink(req)
		if not exp.write(sink, ep):
			return False

		Logging.notice("%s: returned %i events and %i origins (total " \
		               "objects/bytes: %i/%i)" % (ro.service, ep.eventCount(),
		               ep.originCount(), objCount, sink.written))
		utils.accessLog(req, ro, http.OK, sink.written, None)
		return True


	#---------------------------------------------------------------------------
	def _findEvents(self, ep, ro, dbq):
		db = Application.Instance().database()
		def _T(name):
			return db.convertColumnName(name)
		def _time(time):
			return db.timeToString(time)

		reqMag = ro.mag or (ro.orderBy and ro.orderBy.startswith("magnitude"))
		reqDist = ro.geo and ro.geo.bCircle
		colPID = _T("publicID")
		colTime = _T("time_value")
		colMag = _T("magnitude_value")
		if reqMag:
			colOrderBy = "m.%s" % colMag
		else:
			colOrderBy = "o.%s" % colTime

		bBox = None
		if ro.geo:
			colLat, colLon = _T("latitude_value"), _T("longitude_value")
			if ro.geo.bBox:
				bBox = ro.geo.bBox
			else:
				bBox = ro.geo.bCircle.calculateBBox()

		# SELECT --------------------------------
		q = "SELECT DISTINCT pe.%s, e.*, %s" % (colPID, colOrderBy)
		if reqDist: # Great circle distance calculated by Haversine formula
			c = ro.geo.bCircle
			q += ", DEGREES(ACOS(" \
			     "COS(RADIANS(o.%s)) * COS(RADIANS(%s)) * " \
			     "COS(RADIANS(o.%s) - RADIANS(%s)) + SIN(RADIANS(o.%s)) * " \
			     "SIN(RADIANS(%s)))) AS distance" % (
			     colLat, c.lat, colLon, c.lon, colLat, c.lat)

		# FROM ----------------------------------
		q += " FROM Event AS e, PublicObject AS pe" \
		     ", Origin AS o, PublicObject AS po"
		if reqMag:
			q += ", Magnitude AS m, PublicObject AS pm"

		# WHERE ---------------------------------
		q += " WHERE e._oid = pe._oid"

		# event information filter
		if ro.contributors:
			q += " AND e.%s AND upper(e.%s) IN('%s')" % (
			     _T("creationinfo_used"), _T("creationinfo_agencyid"),
			     "', '".join(ro.contributors).upper())

		# origin information filter
		q += " AND o._oid = po._oid AND po.%s = e.%s" % (
		       colPID, _T("preferredOriginID"))

		# time
		if ro.time:
			colTimeMS = _T("time_value_ms")
			if ro.time.start is not None:
				t = _time(ro.time.start)
				ms = ro.time.start.microseconds()
				q += " AND (o.%s > '%s' OR (o.%s = '%s' AND o.%s >= %i)) " % (
				     colTime, t, colTime, t, colTimeMS, ms)
			if ro.time.end is not None:
				t = _time(ro.time.end)
				ms = ro.time.end.microseconds()
				q += " AND (o.%s < '%s' OR (o.%s = '%s' AND o.%s <= %i))" % (
				     colTime, t, colTime, t, colTimeMS, ms)

		# bounding box
		if bBox:
			if bBox.minLat is not None:
				q += " AND o.%s >= %s" % (colLat, bBox.minLat)
			if bBox.maxLat is not None:
				q += " AND o.%s <= %s" % (colLat, bBox.maxLat)
			if bBox.dateLineCrossing():
				q += " AND (o.%s >= %s OR o.%s <= %s)" % (
				     colLon, bBox.minLon, colLon, bBox.maxLon)
			else:
				if bBox.minLon is not None:
					q += " AND o.%s >= %s" % (colLon, bBox.minLon)
				if bBox.maxLon is not None:
					q += " AND o.%s <= %s" % (colLon, bBox.maxLon)

		# depth
		if ro.depth:
			q += " AND o.%s" % _T("depth_used")
			colDepth = _T("depth_value")
			if ro.depth.min is not None:
				q += " AND o.%s >= %s" % (colDepth, ro.depth.min)
			if ro.depth.max is not None:
				q += " AND o.%s <= %s" % (colDepth, ro.depth.max)

		# updated after
		if ro.updatedAfter:
			t = _time(ro.updatedAfter)
			ms = ro.updatedAfter.microseconds()
			colCTime   = _T("creationinfo_creationtime")
			colCTimeMS = _T("creationinfo_creationtime_ms")
			colMTime   = _T("creationinfo_modificationtime")
			colMTimeMS = _T("creationinfo_modificationtime_ms")
			tFilter = "(o.%s > '%s' OR (o.%s = '%s' AND o.%s > %i))"

			q += " AND ("
			q += tFilter % (colCTime, t, colCTime, t, colCTimeMS, ms) + " OR "
			q += tFilter % (colMTime, t, colMTime, t, colMTimeMS, ms) + ")"

		# magnitude information filter
		if reqMag:
			q += " AND m._oid = pm._oid AND "
			if ro.mag and ro.mag.type:
				# join magnitude table on oID of origin and magnitude type
				q += "m._parent_oid = o._oid AND m.%s = '%s'" % (_T("type"),
				                                                 ro.mag.type)
			else:
				# join magnitude table on preferred magnitude id of event
				q += "pm.%s = e.%s" % (colPID, _T("preferredMagnitudeID"))

			if ro.mag and ro.mag.min is not None:
				q += " AND m.%s >= %s" % (colMag, ro.mag.min)
			if ro.mag and ro.mag.max is not None:
				q += " AND m.%s <= %s" % (colMag, ro.mag.max)

		# ORDER BY ------------------------------
		q += " ORDER BY %s" % colOrderBy
		if ro.orderBy and ro.orderBy.endswith("-asc"):
			q += " ASC"
		else:
			q += " DESC"

		# SUBQUERY distance (optional) ----------
		if reqDist:
			q = "SELECT * FROM (%s) AS subquery WHERE distance " % q
			c = ro.geo.bCircle
			if c.minRad is not None:
				q += ">= %s" % c.minRad
			if c.maxRad is not None:
				if c.minRad is not None:
					q += " AND distance "
				q += "<= %s" % c.maxRad

		# LIMIT/OFFSET --------------------------
		if ro.limit is not None or ro.offset is not None:
			# Postgres allows to omit the LIMIT parameter for offsets, MySQL
			# does not. According to the MySQL manual a very large number should
			# be used for this case.
			l = DBMaxUInt
			if ro.limit is not None:
				l = ro.limit
			else:
				l = DBMaxUInt
			q += " LIMIT %i" % l
			if ro.offset is not None:
				q += " OFFSET %i" % ro.offset

		Logging.debug("event query: %s" % q)

		for e in dbq.getObjectIterator(q, DataModel.Event.TypeInfo()):
			ep.add(DataModel.Event.Cast(e))


