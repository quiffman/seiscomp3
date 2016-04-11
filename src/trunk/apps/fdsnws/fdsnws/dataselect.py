################################################################################
# Copyright (C) 2013 by gempa GmbH
#
# FDSNDataSelect -- Implements the fdsnws-dataselect Web service, see
#   http://www.fdsn.org/webservices/
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

from twisted.cred import portal
from twisted.protocols.basic import FileSender
from twisted.web import http, resource, server

from zope.interface import implements

from seiscomp3 import DataModel, Logging
from seiscomp3.Client import Application
from seiscomp3.Core import Array, Record, Time, ValueException
from seiscomp3.IO import RecordInput, RecordStream

from http import HTTP, NoResource
from request import RequestOptions
import utils


################################################################################
class _DataSelectRequestOptions(RequestOptions):

	MinTime        = Time(0, 1)
	PQuality       = "quality"
	PMinimumLength = "minimumlength"
	PLongestOnly   = "longestonly"


	#---------------------------------------------------------------------------
	def __init__(self, args=None):
		RequestOptions.__init__(self, args)
		self.service = "fdsnws-dataselect"

		self.streams = []
		self.quality = "B"
		self.minimumlength = 0.0
		self.longestonly = False


	#---------------------------------------------------------------------------
	def checkTimes(self, realtimeGap):
		maxEndTime = Time(self.accessTime)
		if realtimeGap is not None:
			maxEndTime -= Time(realtimeGap, 0)

		for ro in self.streams:
			# create time if non was specified
			if ro.time is None:
				ro.time = RequestOptions.Time()
			# restrict time to 1970 - now
			if ro.time.start is None or ro.time.start < self.MinTime:
				ro.time.start = self.MinTime
			if ro.time.end is None or ro.time.end > maxEndTime:
				ro.time.end = maxEndTime

		# remove items with start time >= end time
		self.streams = [ x for x in self.streams if x.time.start < x.time.end ]


	#---------------------------------------------------------------------------
	def parseQuality(self, value):
		value = value.upper()
		if value in ('B', 'D', 'M', 'Q', 'R'):
			self.quality = value
		else:
			raise ValueError, "Invalid value in parameter: " + self.PQuality


	#---------------------------------------------------------------------------
	def parseGET(self):
		# quality (optional), currently not supported
		if self.PQuality in self._args:
			self.parseQuality(self._args[self.PQuality][0])

		# minimumlength(optional), currently not supported
		if self.PMinimumLength in self._args:
			self.minimumlength = self.parseFloat(self.PMinimumLength, 0)

		# longestonly (optional), currently not supported
		if self.PLongestOnly in self._args:
			self.longestonly = self.parseBoolean(self.PLongestOnly)

		# channel filter (optional)
		self.parseChannel()
		self.parseTime()

		# this request option is the one and only
		self.streams.append(self)


	#---------------------------------------------------------------------------
	def _getValue(self, line):
		toks = line.split("=", 1)
		if len(toks) < 2:
			raise ValueError, "Invalid key=value pair"
		return toks[1].strip()


	#---------------------------------------------------------------------------
	def parsePOST(self, content):
		nLine = 0

		for line in content:
			nLine += 1
			line = line.strip()

			# ignore empty and comment lines
			if len(line) == 0 or line[0] == '#':
				continue

			# optional key=value parameters
			try:
				# quality (optional), currently not supported
				if line.lower().startswith(self.PQuality):
					self.parseQuality(self._getValue(line))
					continue
				# minimumlength(optional), currently not supported
				if line.lower().startswith(self.PMinimumLength):
					value = self._getValue(line).lower()
					try:
						p.minimumlength = float(self._args[key][0])
					except ValueError:
						raise ValueError, "Invalid float value in " \
						                  "parameter: " + self.PMinimumLength
					continue
				# longestonly (optional), currently not supported
				if line.lower().startswith(self.PLongestOnly):
					value = self._getValue(line).lower()
					if value in self.BooleanTrueValues:
						self.longestonly = True
					elif value not in self.BooleanFalseValues:
						raise ValueError, "Invalid boolean value in " \
						                  "parameter: " + self.PLongestOnly
					continue
			except ValueError, e:
				raise ValueError, "%s (line %i)" % (str(e), nLine)

			# stream parameters
			toks = line.upper().split()
			nToks = len(toks)
			if nToks != 5 and nToks != 6:
				raise ValueError, "Invalid number of stream components in " \
				                  "line %i" % nLine

			ro = RequestOptions()

			# net, sta, loc, cha
			ro.channel = RequestOptions.Channel()
			ro.channel.net = toks[0].split(",")
			ro.channel.sta = toks[1].split(",")
			ro.channel.loc = toks[2].split(",")
			ro.channel.cha = toks[3].split(",")

			msg = "Invalid %s value in line %i"
			for net in ro.channel.net:
				if ro.ChannelChars(net):
					raise ValueError, msg % ("network", nLine)
			for sta in ro.channel.sta:
				if ro.ChannelChars(sta):
					raise ValueError, msg % ("station", nLine)
			for loc in ro.channel.loc:
				if loc != "--" and ro.ChannelChars(loc):
					raise ValueError, msg % ("location", nLine)
			for cha in ro.channel.cha:
				if ro.ChannelChars(cha):
					raise ValueError, msg % ("channel", nLine)

			# start/end time
			ro.time = RequestOptions.Time()
			ro.time.start = Time()
			for fmt in RequestOptions.TimeFormats:
				if ro.time.start.fromString(toks[4], fmt): break
			if len(toks) > 5:
				ro.time.end = Time()
				for fmt in RequestOptions.TimeFormats:
					if ro.time.end.fromString(toks[5], fmt): break

			Logging.debug("ro: %s.%s.%s.%s %s" % (ro.channel.net,
			              ro.channel.sta, ro.channel.loc, ro.channel.cha, ro.time.start.iso()))
			self.streams.append(ro)

		if len(self.streams) == 0:
			raise ValueError, "At least one stream line is required"


################################################################################
class _WaveformProducer:
	def __init__(self, req, ro, rsInput, fileName):
		self.req = req
		self.ro = ro
		self.rsInput = rsInput
		self.fileName = fileName
		self.written = 0

	def resumeProducing(self):
		rec = None

		try: rec = self.rsInput.next()
		except Exception, e: Logging.warning("%s" % str(e))

		if self.written == 0:
			# read first record to test if any data exists at all
			if not rec:
				msg = "No waveform data found"
				self.req.write(HTTP.renderErrorPage(self.req, http.NO_CONTENT,
				               msg, self.ro))
				self.req.unregisterProducer()
				self.req.finish()
				return

			self.req.setHeader("Content-Type", "application/vnd.fdsn.mseed")
			self.req.setHeader("Content-Disposition", "attachment; " \
			                   "filename=%s" % self.fileName)

		if not rec:
			self.req.unregisterProducer()
			Logging.notice("%s: returned %i bytes of mseed data" % (
			               self.ro.service, self.written))
			utils.accessLog(self.req, self.ro, http.OK, self.written, None)
			self.req.finish()
			return

		data = rec.raw().str()
		self.req.write(data)
		self.written += len(data)

	def stopProducing(self): pass



################################################################################
class FDSNDataSelectRealm(object):
	implements(portal.IRealm)

	#---------------------------------------------------------------------------
	def requestAvatar(self, avatarId, mind, *interfaces):
		if resource.IResource in interfaces:
			return resource.IResource, FDSNDataSelect(avatarId), lambda: None
		raise NotImplementedError()



################################################################################
class FDSNDataSelect(resource.Resource):

	isLeaf = True

	#---------------------------------------------------------------------------
	def __init__(self, userName=None):
		resource.Resource.__init__(self)
		self._rsURL = Application.Instance().recordStreamURL()
		self.userName = userName


	#---------------------------------------------------------------------------
	def render_GET(self, req):
		# Parse and validate POST parameters
		ro = _DataSelectRequestOptions(req.args)
		ro.userName = self.userName
		try:
			ro.parseGET()
		except ValueError, e:
			Logging.warning(str(e))
			return HTTP.renderErrorPage(req, http.BAD_REQUEST, str(e), ro)

		return self._processRequest(req, ro)


	#---------------------------------------------------------------------------
	def render_POST(self, req):
		# Parse and validate POST parameters
		ro = _DataSelectRequestOptions()
		ro.userName = self.userName
		try:
			ro.parsePOST(req.content)
		except ValueError, e:
			Logging.warning(str(e))
			return HTTP.renderErrorPage(req, http.BAD_REQUEST, str(e), ro)

		return self._processRequest(req, ro)


	#---------------------------------------------------------------------------
	def _processRequest(self, req, ro):

		if ro.quality != "B" and ro.quality != "M":
			msg = "Quality other than 'B' or 'M' not supported"
			return HTTP.renderErrorPage(req, http.SERVICE_UNAVAILABLE, msg, ro)

		if ro.minimumlength > 0:
			msg = "Enforcing of minimum record length not supported"
			return HTTP.renderErrorPage(req, http.SERVICE_UNAVAILABLE, msg, ro)

		if ro.longestonly:
			msg = "Limitation to longest segment not supported"
			return HTTP.renderErrorPage(req, http.SERVICE_UNAVAILABLE, msg, ro)

		app = Application.Instance()
		ro.checkTimes(app._realtimeGap)

		# Open record stream
		rs = RecordStream.Open(self._rsURL)
		if rs is None:
			msg = "Could not open record stream"
			return HTTP.renderErrorPage(req, http.SERVICE_UNAVAILABLE, msg, ro)

		maxSamples = None
		if app._samplesM is not None:
			maxSamples = app._samplesM * 1000000
			samples = 0

		# Add request streams
		# iterate over inventory networks
		inv = Application.Instance()._inv
		for s in ro.streams:
			for net in utils.networkIter(inv, s):
				if ro.userName is None and net.restricted():
					continue
				for sta in utils.stationIter(net, s, True, False, False):
					if ro.userName is None and sta.restricted():
						continue
					for loc in utils.locationIter(sta, s):
						for cha in utils.streamIter(loc, s):
							# enforce maximum sample per request restriction
							if maxSamples is not None:
								try:
									n = cha.sampleRateNumerator()
									d = cha.sampleRateDenominator()
								except ValueException:
									msg = "skipping stream without sampling " \
									      "rate definition: %s.%s.%s.%s" % (
									      net.code(), sta.code(), loc.code(),
									      cha.code())
									Logging.waring(msg)
									continue

								# calculate number of samples for requested
								# time window
								diffSec = (s.time.end - s.time.start).length()
								samples += int(diffSec * n / d)
								if samples > maxSamples:
									msg = "maximum number of %sM samples " \
									      "exceeded" % str(app._samplesM)
									return HTTP.renderErrorPage(req,
									       http.REQUEST_ENTITY_TOO_LARGE, msg,
									       ro)

							Logging.debug("adding stream: %s.%s.%s.%s %s - %s" \
							              % (net.code(), sta.code(), loc.code(),
							                 cha.code(), s.time.start.iso(),
							                 s.time.end.iso()))
							rs.addStream(net.code(), sta.code(), loc.code(),
							             cha.code(), s.time.start, s.time.end)

		rsInput = RecordInput(rs, Array.INT, Record.SAVE_RAW)

		# Build output filename
		fileName = "sc3.mseed"

		# Create producer for async IO
		req.registerProducer(_WaveformProducer(req, ro, rsInput, fileName),
		                     False)

		# The request is handled by the deferred object
		return server.NOT_DONE_YET


