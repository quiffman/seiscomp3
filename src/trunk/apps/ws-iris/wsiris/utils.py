################################################################################
# Copyright (C) 2012 by gempa GmbH
#
# Common utility functions
#
# Author:  Stephan Herrnkind
# Email:   herrnkind@gempa.de
################################################################################

import traceback

from twisted.internet import reactor
from twisted.web import http

from seiscomp3 import Logging
from seiscomp3.IO import ExportSink

from http import HTTP


#-------------------------------------------------------------------------------
# Filtered network iterator which yields networks matching request options
def networkIter(inv, ro):
	for i in xrange(inv.networkCount()):
		net = inv.network(i)

		if not ro.channel or ro.channel.matchNet(net.code()):
			yield net

#-------------------------------------------------------------------------------
# Filtered station iterator which yields stations of a particular network
# matching request options
def stationIter(net, ro):
	for i in xrange(net.stationCount()):
		sta = net.station(i)
		# station code
		if ro.channel and not ro.channel.matchSta(sta.code()):
			continue

		# geographic location
		if ro.geo:
			try:
				lat = sta.latitude()
				lon = sta.longitude()
			except: continue
			if not ro.geo.match(lat, lon):
				continue

		# start and end time
		if ro.time:
			try: end = sta.end()
			except: end = None
			if not ro.time.match(sta.start(), end):
				continue

		yield sta


#-------------------------------------------------------------------------------
# Filtered location iterator which yields locations of a particular station
# matching request options
def locationIter(sta, ro):
	for i in xrange(sta.sensorLocationCount()):
		loc = sta.sensorLocation(i)
		if not ro.channel or ro.channel.matchLoc(loc.code()):
			yield loc

#-------------------------------------------------------------------------------
# Filtered stream iterator which yields channels of a particular location
# matching request options
def streamIter(loc, ro):
	for i in xrange(loc.streamCount()):
		stream = loc.stream(i)
		if not ro.channel or ro.channel.matchCha(stream.code()):
			yield stream


#-------------------------------------------------------------------------------
# Thread-safe write of data using reactor main thread
def writeTS(req, data):
	reactor.callFromThread(req.write, data)


#-------------------------------------------------------------------------------
# Finish served requests
def onRequestServed(success, req):
	if req._disconnected:
		Logging.debug("Request aborted")
		return

	Logging.debug("Request %s" % (
	              "successfully served" if success else "failed"))
	reactor.callFromThread(req.finish)


#-------------------------------------------------------------------------------
# Handle request errors
def onRequestError(failure, req):
	Logging.error("%s %s" % (failure.getErrorMessage(),
	              traceback.format_tb(failure.getTracebackObject())))

	reactor.callFromThread(req.processingFailed, failure)
	return failure


#-------------------------------------------------------------------------------
# Renders error page if the result set exceeds the configured maximum number
# objects
def checkObjects(req, objCount, maxObj):
	if objCount > maxObj:
		msg = "The result set of your request exceeds the configured maximum " \
		      "number of objects (%i). Refine your request parameters." % maxObj
		writeTS(req, HTTP.renderErrorPage(req, http.REQUEST_ENTITY_TOO_LARGE,
		                                  msg))
		return False

	return True


################################################################################
class WaveformProducer:
	def __init__(self, req, rsInput, fileName):
		self.rsInput = rsInput
		self.fileName = fileName
		self.req = req
		self.initialized = False

	def resumeProducing(self):
		rec = None

		try: rec = self.rsInput.next()
		except Exception, e: Logging.warning("%s" % str(e)) # e.g. ArchiveException

		if not self.initialized:
			self.initialized = True
			# read first record to test if any data exists at all

			if not rec:
				msg = "No waveform data found"
				self.req.write(HTTP.renderErrorPage(self.req, http.NOT_FOUND, msg))
				self.req.unregisterProducer()
				self.req.finish()
				return

			self.req.setHeader("Content-Type", "application/vnd.fdsn.mseed")
			self.req.setHeader("Content-Disposition", "attachment; filename=%s" % \
			                   self.fileName)

		if not rec:
			self.req.unregisterProducer()
			self.req.finish()
			return

		self.req.write(rec.raw().str())

	def stopProducing(self): pass


################################################################################
class Sink(ExportSink):
	def __init__(self, request):
		ExportSink.__init__(self)
		self.request = request

	def write(self, data, size):
		if self.request._disconnected:
			return -1
		writeTS(self.request, data[:size])
		return size

