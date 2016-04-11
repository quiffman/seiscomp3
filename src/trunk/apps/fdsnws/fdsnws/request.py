################################################################################
# Copyright (C) 2013 by gempa GmbH
#
# RequestOptions -- HTTP GET request parameters
#
# Author:  Stephan Herrnkind
# Email:   herrnkind@gempa.de
################################################################################

import fnmatch, math, re, string, sys

from seiscomp3.Core import Time
from seiscomp3 import Logging, Math

class RequestOptions:

	ChannelChars = re.compile(r'[^A-Z0-9*?]').search
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
			self.simpleTime  = True
			self.start       = None
			self.end         = None
			# window time only
			self.startBefore = None
			self.startAfter  = None
			self.endBefore   = None
			self.endAfter    = None

		def match(self, start, end):
			# simple time
			if self.simpleTime:
				return \
					(not self.start or (start and start >= self.start)) and \
					(not self.end or (end and end <= self.end))

			# window time
			return \
				(not self.startBefore or (start and start < self.startBefore)) and \
				(not self.startAfter or (start and start > self.startAfter)) and \
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
				self.lat    = None
				self.lon    = None
				self.minRad = None
				self.maxRad = None

			#-------------------------------------------------------------------
			# Calculates outer bounding box
			def calculateBBox(self):
				def rad(degree):
					return math.radians(degree)
				def deg(radians):
					return math.degrees(radians)

				b = RequestOptions.Geo.BBox()
				if self.maxRad is None or self.maxRad >= 180:
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
			if self.bBox is not None:
				b = self.bBox
				if b.minLat is not None and lat < b.minLat: return False
				if b.maxLat is not None and lat > b.maxLat: return False
				# date line crossing if minLon > maxLon
				if b.dateLineCrossing():
					return lon >= b.minLon or lon <= b.maxLon
				if b.minLon is not None and lon < b.minLon: return False
				if b.maxLat is not None and lon > b.maxLon: return False
				return True
			elif self.bCircle:
				c = self.bCircle
				dist = Math.delazi(c.lat, c.lon, lat, lon)
				if c.minRad is not None and dist[0] < c.minRad: return False
				if c.maxRad is not None and dist[0] > c.maxRad: return False
				return True

			return False


	#---------------------------------------------------------------------------
	def __init__(self, args=None):
		self.service    = ""
		self.accessTime = Time.GMT()
		self.userName   = None

		self.channel = None
		self.time    = None
		self.geo     = None

		# transform keys to lower case
		self._args = {}
		if args is not None:
			for key in args.keys():
				self._args[key.lower()] = args[key]


	#---------------------------------------------------------------------------
	def parseChannel(self):
		c = RequestOptions.Channel()

		if "net" in self._args:
			c.net = self.parseChannelChars("net")
		elif "network" in self._args:
			c.net = self.parseChannelChars("network")

		if "sta" in self._args:
			c.sta = self.parseChannelChars("sta")
		elif "station" in self._args:
			c.sta = self.parseChannelChars("station")

		if "loc" in self._args:
			c.loc = self.parseChannelChars("loc", True)
		elif "location" in self._args:
			c.loc = self.parseChannelChars("location", True)

		if "cha" in self._args:
			c.cha = self.parseChannelChars("cha")
		elif "channel" in self._args:
			c.cha = self.parseChannelChars("channel")

		if c.net is not None or c.sta is not None or \
		   c.loc is not None or c.cha is not None:
			self.channel = c


	#---------------------------------------------------------------------------
	def parseTime(self, parseWindowTime=False):
		t = RequestOptions.Time()

		# start[time]
		if "start" in self._args:
			t.start = self.parseTimeStr("start")
		elif "starttime" in self._args:
			t.start = self.parseTimeStr("starttime")

		# end[time]
		if "end" in self._args:
			t.end = self.parseTimeStr("end")
		elif "endtime" in self._args:
			t.end = self.parseTimeStr("endtime")

		# if we do not parse window time we can stop here
		simpleTime = t.start is not None or t.end is not None
		if not parseWindowTime:
			if simpleTime:
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

		windowTime = t.startBefore is not None or t.startAfter is not None or \
		             t.endBefore is not None or t.endAfter is not None
		if simpleTime and windowTime:
			raise ValueError, "Simple time and window time parameters may " \
			                  "not be combined"
		if simpleTime:
			self.time = t
			self.time.simpleTime = True
		elif windowTime:
			self.time = t
			self.time.simpleTime = False


	#---------------------------------------------------------------------------
	def parseGeo(self):
		# bounding box (optional)
		hasBBoxParam = False
		b = RequestOptions.Geo.BBox()
		if "minlat" in self._args:
			b.minLat = self.parseFloat("minlat", -90, 90)
		elif "minlatitude" in self._args:
			b.minLat = self.parseFloat("minlatitude", -90, 90)

		if "maxlat" in self._args:
			b.maxLat = self.parseFloat("maxlat", -90, 90)
		elif "maxlatitude" in self._args:
			b.maxLat = self.parseFloat("maxlatitude", -90, 90)
		if b.minLat is not None and b.maxLat is not None and \
		   b.minLat > b.maxLat:
			raise ValueError, "minlatitude exceeds maxlatitude"

		if "minlon" in self._args:
			b.minLon = self.parseFloat("minlon", -180, 180)
		elif "minlongitude" in self._args:
			b.minLon = self.parseFloat("minlongitude", -180, 180)

		if "maxlon" in self._args:
			b.maxLon = self.parseFloat("maxlon", -180, 180)
		elif "maxlongitude" in self._args:
			b.maxLon = self.parseFloat("maxlongitude", -180, 180)

		hasBBoxParam = b.minLat is not None or b.maxLat is not None or \
		               b.minLon is not None or b.maxLon is not None

		# bounding circle (optional)
		c = RequestOptions.Geo.BCircle()
		if "lat" in self._args:
			c.lat = self.parseFloat("lat", -90, 90)
		elif "latitude" in self._args:
			c.lat = self.parseFloat("latitude", -90, 90)

		if "lon" in self._args:
			c.lon = self.parseFloat("lon", -180, 180)
		elif "longitude" in self._args:
			c.lon = self.parseFloat("longitude", -180, 180)

		if "minradius" in self._args:
			c.minRad = self.parseFloat("minradius", 0, 180)
		if "maxradius" in self._args:
			c.maxRad = self.parseFloat("maxradius", 0, 180)
		if c.minRad is not None and c.maxRad is not None and \
		   c.minRad > c.maxRad:
			raise ValueError, "minradius exceeds maxradius"

		hasBCircleRadParam = c.minRad is not None or c.maxRad is not None
		hasBCircleParam = c.lat is not None or c.lon is not None or \
		                  hasBCircleRadParam

		# bounding box and bounding circle may not be combined
		if hasBBoxParam and hasBCircleParam:
			raise ValueError, "Bounding box and bounding circle parameters " \
			                  "may not be combined"
		elif hasBBoxParam:
			self.geo = RequestOptions.Geo()
			self.geo.bBox = b
		elif hasBCircleRadParam:
			self.geo = RequestOptions.Geo()
			if c.lat is None:
				c.lat = .0
			if c.lon is None:
				c.lon = .0
			self.geo.bCircle = c


	#---------------------------------------------------------------------------
	def _assertValueRange(self, key, v, minValue, maxValue):
		if (minValue is not None and v < minValue) or \
		   (maxValue is not None and v > maxValue):
			minStr, maxStr = "-inf", "inf"
			if minValue is not None:
				minStr = str(minValue)
			if maxValue is not None:
				maxStr = str(maxValue)
			raise ValueError, "Parameter not in domain [%s,%s]: %s" % (
			                  minStr, maxStr, key)


	#---------------------------------------------------------------------------
	def parseInt(self, key, minValue=None, maxValue=None):
		try:
			i = int(self._args[key][0])
		except ValueError:
			raise ValueError, "Invalid integer value in parameter: %s" % key
		self._assertValueRange(key, i, minValue, maxValue)
		return i


	#---------------------------------------------------------------------------
	def parseLong(self, key, minValue=None, maxValue=None):
		try:
			l = int(self._args[key][0])
		except ValueError:
			raise ValueError, "Invalid long value in parameter: %s" % key
		self._assertValueRange(key, l, minValue, maxValue)
		return l


	#---------------------------------------------------------------------------
	def parseFloat(self, key, minValue=None, maxValue=None):
		try:
			f = float(self._args[key][0])
		except ValueError:
			raise ValueError, "Invalid float value in parameter: %s" % key
		self._assertValueRange(key, f, minValue, maxValue)
		return f


	#---------------------------------------------------------------------------
	def parseBoolean(self, key):
		value = self._args[key][0].lower()
		if value in self.BooleanTrueValues:
			return True
		if value in self.BooleanFalseValues:
			return False

		raise ValueError, "Invalid boolean value in parameter: %s" % key


	#---------------------------------------------------------------------------
	def parseTimeStr(self, key):
		time = Time()
		value = self._args[key][0]
		for fmt in RequestOptions.TimeFormats:
			if time.fromString(value, fmt):
				break

		if not time.valid():
			raise ValueError, "Invalid date format in parameter: %s" % key

		return time


	#---------------------------------------------------------------------------
	def parseChannelChars(self, key, allowEmpty = False):
		value = self._args[key][0]
		if len(value) == 0:
			return None

		values = []
		for v in value.upper().split(','):
			v = v.strip()
			if allowEmpty and (v == "--" or len(v) == 0):
				values.append("--")
				continue

			if self.ChannelChars(v):
				raise ValueError, "Invalid characters in parameter: %s" % key
			values.append(v)

		return values


