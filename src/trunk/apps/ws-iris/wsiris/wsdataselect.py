################################################################################
# Copyright (C) 2012 by gempa GmbH
#
# WSDataSelect -- Implements the IRIS ws-dataselect Web service, see
#   http://www.iris.edu/ws/dataselect/
#
# Feature notes:
#   - 'quality' request parameter not implemented (information not available in
#     SeisComP)
#   - 'ref' parameter is limited to 'direct'
#
# Author:  Stephan Herrnkind
# Email:   herrnkind@gempa.de
################################################################################

import os

from twisted.protocols.basic import FileSender
from twisted.web import http, resource, server

from seiscomp3 import DataModel, Logging
from seiscomp3.Client import Application
from seiscomp3.Core import Array, Record, Time
from seiscomp3.IO import RecordInput, RecordStream

from http import HTTP, NoResource
from request import RequestOptions
import utils


################################################################################
class _DataSelectRequestOptions(RequestOptions):

	MinTime = Time(0, 1)

	#---------------------------------------------------------------------------
	def __init__(self, args):
		RequestOptions.__init__(self, args)
		self.quality = "B"  # [B, D, M, Q, R]
		self.ref = "direct" # [direct, redirect, url, token, xml]


	#---------------------------------------------------------------------------
	def parse(self):
		self.parseChannel()
		self.parseTime()

		# restrict time to 1970 - now
		if self.time.start < self.MinTime:
			self.time.start = self.MinTime
		now = Time.GMT()
		if self.time.end > now:
			self.time.end = now

		# quality (optional), currently not supported
		if "quality" in self._args:
			value = self._args["quality"].upper()
			if value in ('B', 'D', 'M', 'Q', 'R'):
				self.quality = value
			else:
				raise ValueError, "Invalid value in parameter: quality"

		# ref (optional), currently not supported
		if "ref" in self._args:
			value = self._args["ref"].lower()
			if value in ("direct", "redirect", "url", "token", "xml"):
				self.ref = value
			else:
				raise ValueError, "Invalid value in parameter: ref"



################################################################################
class WSDataSelect(resource.Resource):
	isLeaf = True

	#---------------------------------------------------------------------------
	def __init__(self):
		resource.Resource.__init__(self)
		self._sdsPath = Application.Instance()._sdsPath


	#---------------------------------------------------------------------------
	def render_GET(self, req):
		# No archive no service
		if not os.path.isdir(self._sdsPath):
			msg = "SDS archive not found: %s" % self._sdsPath
			return HTTP.renderErrorPage(request, http.SERVICE_UNAVAILABLE, msg)

		# Parse and validate GET parameters
		try:
			ro = _DataSelectRequestOptions(req.args)
			ro.parse()
		except ValueError, e:
			Logging.warning(str(e))
			return HTTP.renderErrorPage(req, http.BAD_REQUEST, str(e))

		if ro.quality != "B" and ro.quality != "M":
			msg = "Quality other than 'B' or 'M' not supported"
			return HTTP.renderErrorPage(req, http.SERVICE_UNAVAILABLE, msg)

		if ro.ref != "direct":
			msg = "Reference other than 'direct' not supported"
			return HTTP.renderErrorPage(req, http.SERVICE_UNAVAILABLE, msg)

		# Open record stream
		rs = RecordStream.Open("sdsarchive://" + self._sdsPath)
		if not rs:
			msg = "Could not open record stream"
			return HTTP.renderErrorPage(req, http.SERVICE_UNAVAILABLE, msg)

		# Add request stream
		c = ro.channel
		rs.addStream(c.net, c.sta, c.loc, c.cha, ro.time.start, ro.time.end)
		rsInput = RecordInput(rs, Array.INT, Record.SAVE_RAW)

		# Build output filename
		fileDateFmt = "%Y.%j.%H.%M.%S"
		fileName = "%s.%s.%s.%s.%s-%s.mseed" % (c.net, c.sta, c.loc, c.cha,
		           ro.time.start.toString(fileDateFmt),
		           ro.time.end.toString(fileDateFmt))

		# Create producer for async IO
		req.registerProducer(utils.WaveformProducer(req, rsInput, fileName), False)

		# The request is handled by the deferred object
		return server.NOT_DONE_YET


