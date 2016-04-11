################################################################################
# Copyright (C) 2012 by gempa GmbH
#
# WSBulkDataSelect -- Implements the IRIS ws-bulkdataselect Web service, see
#   http://www.iris.edu/ws/bulkdataselect/
#
# Feature notes:
#   - 'quality' request parameter not implemented (information not available in
#     SeisComP)
#   - 'minimumlength' parameter is not implemented
#   - 'longestonly' parameter is not implemented
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
class _BulkDataSelectRequestOptions(RequestOptions):

	MinTime = Time(0, 1)

	#---------------------------------------------------------------------------
	def __init__(self, content):
		RequestOptions.__init__(self, {})
		self.content = content
		self.streams = []
		self.selection = None
		self.quality = "B"  # [B, D, M, Q, R]
		self.minimumlength = 0.0
		self.longestonly = False


	#---------------------------------------------------------------------------
	def parse(self):
		# Parse POST data
		sel = []
		for l in self.content:
			l = l.strip()
			toks = l.split(' ', 1)
			if len(toks) < 2: continue
			if toks[0] in ["quality", "minimumlength", "longestonly"]:
				self._args[toks[0]] = toks[1]
			else:
				sel.append(l)
		self._args["selection"] = sel

		# selection (mandatory)
		self.parseSelection()

		# quality (optional), currently not supported
		if "quality" in self._args:
			value = self._args["quality"].upper()
			if value in ('B', 'D', 'M', 'Q', 'R'):
				self.quality = value
			else:
				raise ValueError, "Invalid value in parameter: quality"

		# minimumlength(optional), currently not supported
		if "minimumlength" in self._args:
			self.minimumlength = self.parseFloat("minimumlength", 0)

		# longestonly (optional), currently not supported
		if "longestonly" in self._args:
			self.longestonly = self.parseBoolean("longestonly")


	#---------------------------------------------------------------------------
	def parseSelection(self):
		now = Time.GMT()
		nLine = 0
		for line in self._args["selection"]:
			nLine += 1
			toks = line.split(' ')
			if len(toks) != 6:
				raise ValueError, "Invalid number of stream components in " \
				                  "line %i of parameter: selection" % nLine

			# net, sta, loc, cha
			msg = "Invalid %s value in line %i of parameter: selection"
			if self.ChannelChars(toks[0]):
				raise ValueError, msg % ("network", nLine)
			if self.ChannelChars(toks[1]):
				raise ValueError, msg % ("station", nLine)
			if toks[2] == "--":
				toks[2] = ""
			elif self.ChannelChars(toks[2]):
				raise ValueError, msg % ("location", nLine)
			if self.ChannelChars(toks[3]):
				raise ValueError, msg % ("channel", nLine)

			# start/end time
			start, end = Time(), Time()
			for fmt in RequestOptions.TimeFormats:
				if start.fromString(toks[4], fmt): break
			for fmt in RequestOptions.TimeFormats:
				if end.fromString(toks[5], fmt): break
			if not start.valid():
				raise ValueError, msg % ("start time", nLine)
			if not end.valid():
				raise ValueError, msg % ("end time", nLine)

			# restrict time to 1970 - now
			if start < self.MinTime:
				start = self.MinTime
			if end > now:
				end = now

			self.streams.append((toks[0], toks[1], toks[2], toks[3],
			                     start, end))

		if len(self.streams) == 0:
			raise ValueError, "Add least one stream line is required in " \
			                  "parameter: selection"

################################################################################
class WSBulkDataSelect(resource.Resource):
	isLeaf = True

	#---------------------------------------------------------------------------
	def __init__(self):
		resource.Resource.__init__(self)
		self._sdsPath = Application.Instance()._sdsPath


	#---------------------------------------------------------------------------
	def render_POST(self, req):
		# No archive no service
		if not os.path.isdir(self._sdsPath):
			msg = "SDS archive not found: %s" % self._sdsPath
			return HTTP.renderErrorPage(request, http.SERVICE_UNAVAILABLE, msg)

		# Parse and validate POST parameters
		try:
			ro = _BulkDataSelectRequestOptions(req.content)
			ro.parse()
		except ValueError, e:
			Logging.warning(str(e))
			return HTTP.renderErrorPage(req, http.BAD_REQUEST, str(e))

		if ro.quality != "B" and ro.quality != "M":
			msg = "Quality other than 'B' or 'M' not supported"
			return HTTP.renderErrorPage(req, http.SERVICE_UNAVAILABLE, msg)

		if ro.minimumlength > 0:
			msg = "Enforcing of minimum record length not supported"
			return HTTP.renderErrorPage(req, http.SERVICE_UNAVAILABLE, msg)

		if ro.longestonly:
			msg = "Limitation to longest segment not supported"
			return HTTP.renderErrorPage(req, http.SERVICE_UNAVAILABLE, msg)

		# Open record stream
		rs = RecordStream.Open("sdsarchive://" + self._sdsPath)
		if not rs:
			msg = "Could not open record stream"
			return HTTP.renderErrorPage(req, http.SERVICE_UNAVAILABLE, msg)

		# Add request streams
		for s in ro.streams:
			rs.addStream(s[0], s[1], s[2], s[3], s[4], s[5])

		rsInput = RecordInput(rs, Array.INT, Record.SAVE_RAW)

		# Build output filename
		fileName = "foo.mseed"

		# Create producer for async IO
		req.registerProducer(utils.WaveformProducer(req, rsInput, fileName), False)

		# The request is handled by the deferred object
		return server.NOT_DONE_YET
