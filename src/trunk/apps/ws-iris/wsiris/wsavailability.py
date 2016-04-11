################################################################################
# Copyright (C) 2012 by gempa GmbH
#
# WSAvailability -- Implements the IRIS ws-availability Web service, see
#   http://www.iris.edu/ws/availability/
#
# Feature notes:
#   - data availability is only based on SDS file names (not miniSEED content),
#     hence date precision is limited to days
#
# Author:  Stephan Herrnkind
# Email:   herrnkind@gempa.de
################################################################################

import os
import fnmatch

from twisted.internet.threads import deferToThread
from twisted.web import http, resource, server

from seiscomp3 import DataModel, Logging
from seiscomp3.Client import Application
from seiscomp3.Core import Time

from http import HTTP, NoResource
from request import RequestOptions
import utils

PACKET_SIZE = 520
HEADER_SIZE = 72
TERM_POS = 7

# Template for 'output=xml', this is not a complete and valid StationXML
# format
SX_MSG_HEAD = """\
<?xml version="1.0" encoding="UTF-8"?>
<StaMessage xmlns="http://www.data.scec.org/xml/station/">
  <Source>SeisComP3</Source>
  <SentDate>%s</SentDate>"""
SX_STA_HEAD = """
  <Station net_code="%s" sta_code="%s">"""
SX_STA_LAT = """
    <Lat>%f</Lat>"""
SX_STA_LON = """
    <Lon>%f</Lon>"""
SX_STA_ELE = """
    <Elevation>%f</Elevation>"""
SX_CHA = """
    <Channel chan_code="%s" loc_code="%s">
      <Availability>
        <Extent start="%s" end="%s"></Extent>
      </Availability>
    </Channel>"""
SX_STA_FOOT = """
  </Station>"""
SX_MSG_FOOT = """
</StaMessage>"""

# Time format for start and end times, same for all output formats
TIME_FMT = "%FT%T"


#################################################################################
#class _SLInfoStreams(protocol.Protocol):
#
#	#---------------------------------------------------------------------------
#	def connectionMade(self):
#		Logging.debug("Requesting stream information")
#		self.buf = ""
#		self.factory.xmlData = ""
#		self.transport.write("INFO STREAMS\n")
#
#
#	#---------------------------------------------------------------------------
#	def dataReceived(self, data):
#		self.buf += data
#		while len(self.buf) >= PACKET_SIZE:
#			if not self.buf.startswith("SLINFO"):
#				self.factory.error = "Invalid response from server"
#				self.transport.loseConnection()
#				return
#
#			# a '*' indicates that more packets are following
#			if self.buf[TERM_POS] == '*':
#				self.factory.xmlData += self.buf[HEADER_SIZE:PACKET_SIZE]
#				self.buf = self.buf[PACKET_SIZE:]
#			else:
#				self.factory.xmlData += self.buf[HEADER_SIZE:PACKET_SIZE] \
#				                        .rstrip('\000')
#				self.buf = ""
#				self.factory.success = True
#				self.transport.loseConnection()
#				return
#
#
#################################################################################
#class _SLFactory(protocol.ClientFactory):
#
#	#---------------------------------------------------------------------------
#	def __init__(self):
#		self.protocol = _SLInfoStreams
#		self.xmlData = ""
#		self.success = False
#		self.error = ""
#		self.deferred = defer.Deferred()
#
#
#	#---------------------------------------------------------------------------
#	def clientConnectionFailed(self, connector, reason):
#		error = "Could not connect to SeedLink: %s" % \
#		        reason.getErrorMessage()
#		Logging.error(error)
#		self.deferred.errback(error);
#
#
#	#---------------------------------------------------------------------------
#	def clientConnectionLost(self, connector, reason):
#		if self.success:
#			Logging.notice("Received station information (%i bytes)" % \
#			               len(self.xmlData))
#			self.deferred.callback(self.xmlData)
#		else:
#			if self.error == "":
#				self.error = reason.getErrorMessage()
#			error = "Could not retrieve station information: %s" % self.error
#			Logging.error(error)
#			self.deferred.errback(error)
#
#
#################################################################################
#class _StreamInfoHandler(sax.ContentHandler):
#	TimeFmt = "%Y/%m/%d %T.%f"
#
#	def __init__(self):
#		self.organization = None
#		self.networks = {}
#
#	def startElement(self, name, attrs):
#		if name == "seedlink":
#			self.organization = str(attrs["organization"])
#
#		if name == "station":
#			self.staName = str(attrs["name"])
#			self.netName = str(attrs["network"])
#			self.streams = []
#
#		if name == "stream":
#			loc = str(attrs["location"])
#			if loc == "":
#				loc = "--"
#			cha = str(attrs["seedname"])
#			start = Time.FromString(str(attrs["begin_time"]), self.TimeFmt)
#			end = Time.FromString(str(attrs["end_time"]), self.TimeFmt)
#			self.streams.append((loc, cha, start, end))
#
#	def endElement(self, name):
#		if name == "station":
#			if not self.netName in self.networks:
#				self.networks[self.netName] = {}
#			self.networks[self.netName][self.staName] = self.streams


################################################################################
class _AvailRequestOptions(RequestOptions):

	#---------------------------------------------------------------------------
	def __init__(self, args):
		RequestOptions.__init__(self, args)
		self.restricted = False
		self.output = "xml" # [xml, query, bulkdataselect]


	#---------------------------------------------------------------------------
	def parse(self):
		self.parseChannelGlobs()
		self.parseTime()
		self.parseGeo()

		# start time is mandatory
		if not self.time or not self.time.start:
			raise ValueError, "Missing mandatory parameter: start[time]"

		# restricted (optional)
		if "restricted" in self._args:
			self.restricted = self.parseBoolean("restricted")

		# output format (optional)
		if "output" in self._args:
			value = self._args["output"].lower()
			if value in ("xml", "query", "bulkdataselect"):
				self.output = value
			else:
				raise ValueError, "Invalid value in parameter: output"


################################################################################
class WSAvailability(resource.Resource):
	isLeaf = True

	#---------------------------------------------------------------------------
	def __init__(self):
		resource.Resource.__init__(self)
		self._sdsPath = Application.Instance()._sdsPath


	#---------------------------------------------------------------------------
	def render_GET(self, request):
		# No archive no service
		if not os.path.isdir(self._sdsPath):
			msg = "SDS archive not found: %s" % self._sdsPath
			return HTTP.renderErrorPage(request, http.SERVICE_UNAVAILABLE, msg)

		# Parse and validate GET parameters
		try:
			ro = _AvailRequestOptions(request.args)
			ro.parse()
		except ValueError, e:
			Logging.warning(str(e))
			return HTTP.renderErrorPage(request, http.BAD_REQUEST, str(e))

		# Process request in separate thread
		d = deferToThread(self._processRequest, request, ro)
		d.addCallback(utils.onRequestServed, request)
		d.addErrback(utils.onRequestError, request)

		# The request is handled by the deferred object
		return server.NOT_DONE_YET


	#---------------------------------------------------------------------------
	def _processRequest(self, req, ro):
		if req._disconnected:
			return False
		xmlOutput = ro.output == "xml"
		if xmlOutput:
			req.setHeader("Content-Type", "text/xml")
			utils.writeTS(req, SX_MSG_HEAD % Time.GMT().toString(TIME_FMT))
		else:
			req.setHeader("Content-Type", "text/plain")
			if ro.output == "query":
				respLine = "net=%s&sta=%s&loc=%s&cha=%s&start=%s&end=%s\n"
			else:
				respLine = "%s %s %s %s %s %s\n"

		streamCount = 0

		# build list of years dependent on start and end time
		years = self._getYearDirs(ro)

		# iterate over inventory networks
		for net in utils.networkIter(Application.Instance()._inv, ro):
			netCode = net.code()

			# iterate over inventory stations of current network
			for sta in utils.stationIter(net, ro):
				if req._disconnected:
					return False

				# restricted: true means all stations, false only non restricted
				if not ro.restricted:
					try: restricted = sta.restricted()
					except: continue
					if restricted:
						continue

				staCode = sta.code()
				xmlStaHeadWritten = False
				streams = self._getStreams(ro, years, netCode, staCode)

				if xmlOutput:
					if len(streams) == 0:
						continue
					utils.writeTS(req, SX_STA_HEAD % (netCode, staCode))
					try: utils.writeTS(req, SX_STA_LAT % sta.latitude())
					except: pass
					try: utils.writeTS(req, SX_STA_LON % sta.longitude())
					except: pass
					try: utils.writeTS(req, SX_STA_ELE % sta.elevation())
					except: pass

				for stream in streams:
					if xmlOutput:
						utils.writeTS(req, SX_CHA % (stream[1], stream[0],
						                   stream[2].toString(TIME_FMT),
						                   stream[3].toString(TIME_FMT)))
					else:
						loc = stream[0] if len(stream[0]) > 0 else "--"
						utils.writeTS(req, respLine % (netCode, staCode, loc,
						              stream[1], stream[2].toString(TIME_FMT),
						              stream[3].toString(TIME_FMT)))

				streamCount += len(streams)

				# end of station: close Station element
				if xmlOutput:
					utils.writeTS(req, SX_STA_FOOT)
		# end of all networks: close StaMesage element
		if xmlOutput:
			utils.writeTS(req, SX_MSG_FOOT)

		Logging.notice("WS-Availability: Returned %i streams" % streamCount)
		return True


	#---------------------------------------------------------------------------
	def _getYearDirs(self, ro):
		entries = os.listdir(self._sdsPath)
		startYear = ro.time.start.toString("%Y")
		endYear = ro.time.end.toString("%Y") if ro.time.end else None
		years = []
		for year in fnmatch.filter(entries, "[1-2][0-9][0-9][0-9]"):
			if not os.path.isdir(os.path.join(self._sdsPath, year)):
				continue
			if year >= startYear and (not endYear or year <= endYear):
				years.append(year)

		years.sort()
		return years


	#---------------------------------------------------------------------------
	# Finds all streams of a particular station which matches time and channel
	# filter. Returns stream list of tuples (location, channel, start time,
	# end time)
	def _getStreams(self, ro, years, net, sta):
		netStaPath = os.path.join(net, sta)
		filePrefix = "%s.%s." % (net, sta)
		filePrefixLen = len(filePrefix)

		streams = []
		files = []
		# Iterate over year archives and collect all existing file names of
		# current station
		for year in years:
			path = os.path.join(self._sdsPath, year, netStaPath)
			if not os.path.isdir(path):
				continue

			# Iterate through
			for cha in fnmatch.filter(os.listdir(path), "*.D"):
				chaPath = os.path.join(path, cha)
				cha = cha[:-2]

				# Match channel filter (if any) and check if directory exists
				if (ro.channel and not ro.channel.matchCha(cha)) or \
				   not os.path.isdir(chaPath):
					continue

				pattern = filePrefix
				pattern += "*.%s.D.%s.???" % (cha, year)
				files.extend(fnmatch.filter(os.listdir(chaPath), pattern))

		files.sort()

		# Iterate over file names
		firstFileToks, lastFileName, stream, streamLen = None, None, None, 0
		for fileName in files:
			# First file of new stream
			if not stream or stream != fileName[:streamLen]:
				# Add previous stream information before switching to new one
				if firstFileToks:
					self._addStream(ro, streams, firstFileToks, lastFileName)

				# New stream information
				firstFileToks = fileName.rsplit('.', 5)
				if len(firstFileToks) != 6:
					firstFileToks = None
					continue
				loc = firstFileToks[1]
				cha = firstFileToks[2]
				stream = filePrefix + loc + '.' + cha + '.'
				streamLen = len(stream)

				# Check if the location is filtered
				if ro.channel and not ro.channel.matchLoc(loc):
					firstFileToks = None
					continue

			if firstFileToks:
				lastFileName = fileName

		# Add last stream
		if firstFileToks:
			self._addStream(ro, streams, firstFileToks, lastFileName)

		return streams


	#---------------------------------------------------------------------------
	def _addStream(self, ro, streams, toks, lastFileName):
		start, end = Time(), Time()
		if start.fromString("%s.%s" % (toks[4], toks[5]), "%Y.%j") and \
		   end.fromString(lastFileName[-8:] + "23:59:59", "%Y.%j%T"):
			# match time
			if ro.time.start > end or \
			   (ro.time.end and ro.time.end < start):
				return

			# limit time to requested time
			if ro.time.start > start:
				start = ro.time.start
			if ro.time.end and ro.time.end < end:
				end = ro.time.end

			streams.append((toks[1], toks[2], start, end))
		else:
			Logging.warning("invalid stream information: %s%s.%s" % (
			                toks[0], toks[1], toks[2]))

