################################################################################
# Copyright (C) 2012 by gempa GmbH
#
# HTTP -- Utility methods which generate HTTP result strings
#
# Author:  Stephan Herrnkind
# Email:   herrnkind@gempa.de
################################################################################

from twisted.web import http, resource, server

from seiscomp3 import Core, Logging


################################################################################
class HTTP:

	#---------------------------------------------------------------------------
	@staticmethod
	def renderErrorPage(request, code, msg):
		html = """\
<html>
	<head><title>%i - %s</title></head>
	<body>
		<h1>%s</h1>
		<p>%s</p>
	</body>
</html>"""

		request.setHeader("Content-Type", "text/html")
		request.setResponseCode(code)

		codeStr = http.RESPONSES[code]
		Logging.warning("Responding with error: %i (%s)" % (code, codeStr))
		return html % (code, codeStr, codeStr, msg)


	#---------------------------------------------------------------------------
	@staticmethod
	def renderNotFound(request):
		msg = "The requested resource does not exist on this server"
		return HTTP.renderErrorPage(request, http.NOT_FOUND, msg)


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

	_version = "Unknown"

	#---------------------------------------------------------------------------
	def setVersion(self, version):
		self._version = version


	#---------------------------------------------------------------------------
	def getResourceFor(self, request):
		Logging.debug("request (%s): %s" % (request.getClientIP(),
		              request.uri))
		request.setHeader("Server", "SeisComP3-WS/%s" % self._version)
		return server.Site.getResourceFor(self, request)
