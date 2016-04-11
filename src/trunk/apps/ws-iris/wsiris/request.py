################################################################################
# Copyright (C) 2012 by gempa GmbH
#
# RequestOptions -- HTTP GET request parameters
#
# Author:  Stephan Herrnkind
# Email:   herrnkind@gempa.de
################################################################################

import fnmatch
import math
import re, string

from seiscomp3.Core import Time
from seiscomp3 import Logging, Math

class RequestOptions:

	ChannelChars = re.compile(r'[^A-Z0-9]').search
	ChannelGlobChars = re.compile(r'[^A-Z0-9*?]').search
	TimeFormats = ( "%FT%T.%f",    # YYYY-MM-DDThh:mm:ss.sss
	                "%Y-%jT%T.%f", # YYYY-DDDThh:mm:ss.sss
	                "%FT%T",       # YYYY-MM-DDThh:mm:ss
	                "%Y-%jT%T",    # YYYY-DDDThh:mm:ss
	                "%FT%R",       # YYYY-MM-DDThh:mm
	                "%Y-%jT%R",    # YYYY-DDDThh:mm
	                "%FT%H",       # YYYY-MM-DDThh
	                "%Y-%jT%H",    # YYYY-DDDThh
	                "%F",          # YYYY-MM-DD
	                "%Y-%j",       # YYYY-DDD
	                "%Y",          # YYYY
	               )
	BooleanTrueValues = ( "1", "true", "t", "yes", "y" )
	BooleanFalseValues = ( "0", "false", "f", "no", "n" )


	#---------------------------------------------------------------------------
	class Channel:
		def __init__(self):
			self.net = None
			self.sta = None
			self.loc = None
			self.cha = None

		def matchNet(self, value):
			return self.match(value, self.net)

		def matchSta(self, value):
			return self.match(value, self.sta)

		def matchLoc(self, value):
			return self.match(value, self.loc, True)

		def matchCha(self, value):
			return self.match(value, self.cha)

		@staticmethod
		def match(value, globList, testEmpty = False):
			if not globList:
				return True

			for glob in globList:
				if testEmpty and value == "" and glob == "--":
					return True
				if fnmatch.fnmatchcase(value, glob):
					return True

			return False


	#---------------------------------------------------------------------------
	class Time:
		def __init__(self):
			self.stationTime = False
			self.start       = None
			self.end         = None
			# station time only
			self.startBefore = None
			self.startAfter  = None
			self.endBefore   = None
			self.endAfter    = None

		def match(self, start, end):
			if not self.stationTime:
				return True

			return \
				(not self.start or (start and start >= self.start)) and \
				(not self.startBefore or (start and start < self.startBefore)) and \
				(not self.startAfter or (start and start > self.startAfter)) and \
				(not self.end or (end and end <= self.end)) and \
				(not self.endBefore or (end and end < self.endBefore)) and \
				(not self.endAfter or (end and end > self.endAfter))


	#---------------------------------------------------------------------------
	class Geo:

		#-----------------------------------------------------------------------
		class BBox:
			def __init__(self):
				self.minLat = None
				self.maxLat = None
				self.minLon = None
				self.maxLon = None

			def dateLineCrossing(self):
				return self.minLon and self.maxLon and self.minLon > self.maxLon

		#-----------------------------------------------------------------------
		class BCircle:
			def __init__(self):
				self.lat    = .0
				self.lon    = .0
				self.minRad = None
				self.maxRad = .0

			#-------------------------------------------------------------------
			# Calculates outer bounding box
			def calculateBBox(self):
				def rad(degree):
					return math.radians(degree)
				def deg(radians):
					return math.degrees(radians)

				b = RequestOptions.Geo.BBox()
				if self.maxRad >= 180:
					return b

				b.minLat = self.lat - self.maxRad
				b.maxLat = self.lat + self.maxRad
				if b.minLat > -90 and b.maxLat < 90:
					dLon = deg(math.asin(math.sin(rad(self.maxRad) /
					                     math.cos(rad(self.lat)))))
					b.minLon = self.lon - dLon
					if b.minLon < -180: b.minLon += 360
					b.maxLon = self.lon + dLon
					if b.maxLon > 180: b.maxLon -= 360
				else:
					# pole within distance: one latitude and no longitude
					# restrictions remains
					if b.minLat <= -90: b.minLat = None
					else: b.maxLat = None
					b.minLon = None
					b.maxLon = None

				return b

		#-----------------------------------------------------------------------
		def __init__(self):
			self.bBox    = None
			self.bCircle = None

		#-----------------------------------------------------------------------
		def match(self, lat, lon):
			if self.bBox:
				b = self.bBox
				if b.minLat and lat < b.minLat: return False
				if b.maxLat and lat > b.maxLat: return False
				# date line crossing if minLon > maxLon
				if b.dateLineCrossing():
					return lon >= b.minLon or lon <= b.maxLon
				if b.minLon and lon < b.minLon: return False
				if b.maxLat and lon > b.maxLon: return False
				return True
			elif self.bCircle:
				c = self.bCircle
				dist = Math.delazi(c.lat, c.lon, lat, lon)
				return dist[0] <= c.maxRad and (
				       not c.minRad or dist[0] >= c.minRad)

			return False


	#---------------------------------------------------------------------------
	def __init__(self, args):
		self.channel    = None
		self.time       = None
		self.geo        = None

		# transform keys to lower case and store only first element of each
		# value list
		self._args = {}
		for key in args.keys():
			self._args[key.lower()] = args[key][0]


	#---------------------------------------------------------------------------
	def parseChannel(self):
		c = self.Channel()

		if "net" in self._args:
			c.net = self._channelValue("net")
		elif "network" in self._args:
			c.net = self._channelValue("network")
		else:
			raise ValueError, "Missing mandatory parameter: net[work]"

		if "sta" in self._args:
			c.sta = self._channelValue("sta")
		elif "station" in self._args:
			c.sta = self._channelValue("station")
		else:
			raise ValueError, "Missing mandatory parameter: sta[tion]"

		if "loc" in self._args:
			c.loc = self._channelValue("loc", True)
		elif "location" in self._args:
			c.loc = self._channelValue("location", True)
		else:
			raise ValueError, "Missing mandatory parameter: loc[ation]"

		if "cha" in self._args:
			c.cha = self._channelValue("cha")
		elif "channel" in self._args:
			c.cha = self._channelValue("channel")
		else:
			raise ValueError, "Missing mandatory parameter: cha[nnel]"

		self.channel = c


	#---------------------------------------------------------------------------
	def parseChannelGlobs(self):
		c = RequestOptions.Channel()
		found = False

		if "net" in self._args:
			c.net = self._channelGlobs("net")
			found = True
		elif "network" in self._args:
			c.net = self._channelGlobs("network")
			found = True

		if "sta" in self._args:
			c.sta = self._channelGlobs("sta")
			found = True
		elif "station" in self._args:
			c.sta = self._channelGlobs("station")
			found = True

		if "loc" in self._args:
			c.loc = self._channelGlobs("loc", True)
			found = True
		elif "location" in self._args:
			c.loc = self._channelGlobs("location", True)
			found = True

		if "cha" in self._args:
			c.cha = self._channelGlobs("cha")
			found = True
		elif "chan" in self._args:
			c.cha = self._channelGlobs("chan")
			found = True
		elif "channel" in self._args:
			c.cha = self._channelGlobs("channel")
			found = True

		if found:
			self.channel = c


	#---------------------------------------------------------------------------
	def parseTime(self, stationTime = False):
		t = RequestOptions.Time()
		t.stationTime = stationTime

		# start[time]
		if not stationTime and "start" in self._args:
			t.start = self.parseTimeStr("start")
		elif "starttime" in self._args:
			t.start = self.parseTimeStr("starttime")

		# end[time]
		if not stationTime and "end" in self._args:
			t.end = self.parseTimeStr("end")
		elif "endtime" in self._args:
			t.end = self.parseTimeStr("endtime")

		# if we do not parse station time we can stop here
		if not stationTime:
			if t.start or t.end:
				self.time = t
			return

		# [start,end][before,after]
		if "startbefore" in self._args:
			t.startBefore = self.parseTimeStr("startbefore")
		if "startafter" in self._args:
			t.startAfter = self.parseTimeStr("startafter")
		if "endbefore" in self._args:
			t.endBefore = self.parseTimeStr("endbefore")
		if "endafter" in self._args:
			t.endAfter = self.parseTimeStr("endafter")

		if t.start or t.end or t.startBefore or t.startAfter or \
		   t.endBefore or t.endAfter:
			self.time = t


	#---------------------------------------------------------------------------
	def parseGeo(self):
		# bounding box (optional), any combination is valid
		hasBBoxParam = False
		b = RequestOptions.Geo.BBox()
		if "minlat" in self._args:
			b.minLat = self.parseFloat("minlat", -90, 90)
			hasBBoxParam = True
		if "maxlat" in self._args:
			b.maxLat = self.parseFloat("maxlat", -90, 90)
			hasBBoxParam = True
		if "minlon" in self._args:
			b.minLon = self.parseFloat("minlon", -180, 180)
			hasBBoxParam = True
		if "maxlon" in self._args:
			b.maxLon = self.parseFloat("maxlon", -180, 180)
			hasBBoxParam = True

		# bounding circle (optional), all parameters except 'minradius' must
		# be given
		hasBCircleParam, bCircleComplete = False, True
		c = RequestOptions.Geo.BCircle()
		if "lat" in self._args and bCircleComplete:
			c.lat = self.parseFloat("lat", -90, 90)
			hasBCircleParam = True
		else:
			bCircleComplete = False

		if "lon" in self._args and bCircleComplete:
			c.lon = self.parseFloat("lon", -180, 180)
			hasBCircleParam = True
		else:
			bCircleComplete = False

		if "maxradius" in self._args and bCircleComplete:
			c.maxRad = self.parseFloat("maxradius", 0, 180)
			hasBCircleParam = True
		else:
			bCircleComplete = False

		if "minradius" in self._args and bCircleComplete:
			c.minRad = self.parseFloat("minradius", 0, c.maxRad)

		# bounding box and bounding circle may not be combined
		if hasBBoxParam and hasBCircleParam:
			raise ValueError, "Bounding box and bounding circle parameters " \
			                  "may not be combined"
		elif hasBBoxParam:
			self.geo = RequestOptions.Geo()
			self.geo.bBox = b
		elif hasBCircleParam:
			if not bCircleComplete:
				raise ValueError, "Not all bounding circle parameters specified"
			self.geo = RequestOptions.Geo()
			self.geo.bCircle = c


	#---------------------------------------------------------------------------
	def parseFloat(self, key, minValue = None, maxValue = None):
		try:
			f = float(self._args[key])
		except ValueError:
			raise ValueError, "Invalid float value in parameter: %s" % key

		if (minValue and f < minValue) or (maxValue and f > maxValue):
			raise ValueError, "Parameter not in domain [%s,%s]: %s" % (
			                  str(minValue) if minValue else "-&infin;",
			                  str(maxValue) if maxValue else "&infin;", key)

		return f


	#---------------------------------------------------------------------------
	def parseBoolean(self, key):
		value = self._args[key].lower()
		if value in self.BooleanTrueValues:
			return True
		if value in self.BooleanFalseValues:
			return False

		raise ValueError, "Invalid value in parameter: %s" % key


	#---------------------------------------------------------------------------
	def parseTimeStr(self, key):
		time = Time()
		value = self._args[key]
		for fmt in RequestOptions.TimeFormats:
			if time.fromString(value, fmt):
				break

		if not time.valid():
			raise ValueError, "Invalid date format in parameter: %s" % key

		return time


	#---------------------------------------------------------------------------
	def _channelValue(self, key, allowEmpty = False):
		value = self._args[key]
		if len(value) == 0:
			raise ValueError, "Empty mandatory parameter: %s" % key

		if allowEmpty and value == "--":
			return ""

		value = value.upper()
		if self.ChannelChars(value):
			raise ValueError, "Invalid characters in parameter: %s" % key

		return value


	#---------------------------------------------------------------------------
	def _channelGlobs(self, key, allowEmpty = False):
		value = self._args[key]
		if len(value) == 0:
			return None

		values = value.upper().split(',')
		for v in values:
			if allowEmpty and v == "--":
				continue

			if self.ChannelGlobChars(v):
				raise ValueError, "Invalid characters in parameter: %s" % key

		return values


