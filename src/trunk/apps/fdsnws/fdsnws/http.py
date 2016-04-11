################################################################################
# Copyright (C) 2013 by gempa GmbH
#
# HTTP -- Utility methods which generate HTTP result strings
#
# Author:  Stephan Herrnkind
# Email:   herrnkind@gempa.de
################################################################################

from twisted.web import http, resource, server

from seiscomp3 import Core, Logging

import utils

VERSION = "1.0.0"

################################################################################
class HTTP:

	#---------------------------------------------------------------------------
	@staticmethod
	def renderErrorPage(request, code, msg, ro=None):
		resp = """\
Error %i: %s

%s

Request:
%s

Request Submitted:
%s

Service Version:
%s
"""

		# rewrite response code if requested an no data was found
		if ro is not None and code == http.NO_CONTENT:
			code = ro.noData

		request.setHeader("Content-Type", "text/plain")
		request.setResponseCode(code)

		codeStr = http.RESPONSES[code]
		Logging.warning("responding with error: %i (%s)" % (code, codeStr))
		date = Core.Time.GMT().toString("%FT%T.%f")
		response = resp % (code, codeStr, msg, request.uri, date, VERSION)
		utils.accessLog(request, ro, code, len(response), msg)
		return response


	#---------------------------------------------------------------------------
	@staticmethod
	def renderNotFound(request):
		msg = "The requested resource does not exist on this server."
		return HTTP.renderErrorPage(request, http.NOT_FOUND, msg)

	#---------------------------------------------------------------------------
	# Renders error page if the result set exceeds the configured maximum number
	# objects
	@staticmethod
	def checkObjects(req, objCount, maxObj):
		if objCount <= maxObj:
			return True

		msg = "The result set of your request exceeds the configured maximum " \
		      "number of objects (%i). Refine your request parameters." % maxObj
		utils.writeTS(req, HTTP.renderErrorPage(
		              req, http.REQUEST_ENTITY_TOO_LARGE, msg))
		return False




################################################################################
class ServiceVersion(resource.Resource):
	isLeaf = True

	#---------------------------------------------------------------------------
	def render(self, request):
		return VERSION


################################################################################
class NoResource(resource.Resource):

	#---------------------------------------------------------------------------
	def render(self, request):
		return HTTP.renderNotFound(request)


	#---------------------------------------------------------------------------
	def getChild(self, chnam, request):
		return self


################################################################################
class Site(server.Site):

	#---------------------------------------------------------------------------
	def getResourceFor(self, request):
		Logging.debug("request (%s): %s" % (request.getClientIP(),
		              request.uri))
		request.setHeader("Server", "SeisComP3-FDSNWS/%s" % VERSION)
		return server.Site.getResourceFor(self, request)
