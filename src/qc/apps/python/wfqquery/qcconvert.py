#***************************************************************************** 
# qccopydb.py
#
# copy qc database from src to dst database
#
# (c) 2010 Mathias Hoffmann, GFZ Potsdam
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any later
# version. For more information, see http://www.gnu.org/
#*****************************************************************************


import sys
import re
from seiscomp3 import Client, IO, DataModel, Core


#----------------------------------------------------------------------------------------------------
#----------------------------------------------------------------------------------------------------
class WfqQuery(Client.Application):


#----------------------------------------------------------------------------------------------------
	def __init__(self, argc, argv):
		Client.Application.__init__(self, argc, argv)

		self.setMessagingEnabled(False)
		self.setDatabaseEnabled(False, False)
		self.setAutoApplyNotifierEnabled(False)
		self.setInterpretNotifierEnabled(False)
#----------------------------------------------------------------------------------------------------


#----------------------------------------------------------------------------------------------------
	def createCommandLineDescription(self):
		self.commandline().addGroup("Convert")
		self.commandline().addStringOption("Convert", "source", "source database","mysql://sysop:sysop@localhost/seiscomp3");
		self.commandline().addStringOption("Convert", "target", "target database","mysql://sysop:sysop@localhost/seiscomp3n");

		self.commandline().addGroup("Query")
		self.commandline().addStringOption("Query", "startTime,b", "start time: YYYY-MM-DD")
		self.commandline().addStringOption("Query", "endTime,e", "end time: YYYY-MM-DD")

		return True
#----------------------------------------------------------------------------------------------------


#----------------------------------------------------------------------------------------------------
	def validateParameters(self):
		try:
			self.source = self.commandline().optionString("source")
			self.target = self.commandline().optionString("target")
		except: pass

		try:
			self.startTime = Core.Time().FromString(self.commandline().optionString("startTime"), "%Y-%m-%d")
			self.endTime = Core.Time().FromString(self.commandline().optionString("endTime"), "%Y-%m-%d")
		except:
			print "Please specify startTime and endTime!"
			return False

		return True
#----------------------------------------------------------------------------------------------------


#----------------------------------------------------------------------------------------------------
	def run(self):

		db1 = IO.DatabaseInterface.Open(self.source)
		if not db1:
			print "No SOURCE database connection: %s" % self.source
			return False
		dbSource = DataModel.DatabaseQuery(db1)

		db2 = IO.DatabaseInterface.Open(self.target)
		if not db2:
			print "No Target database connection: %s" % self.target
			return False
		dba = DataModel.DatabaseArchive(db2)
		dbTarget = DataModel.DatabaseObjectWriter(dba, 100)

		print "starting query ..."
		query = dbSource.getWaveformQuality(self.startTime, self.endTime)

		print "writing to destination ..."
		qc = DataModel.QualityControl()
		for obj in query:
			sys.stderr.write(".")
			sys.stderr.flush()
			wfq = DataModel.WaveformQuality.Cast(obj)
			if wfq:
				wfq.setParent(qc)
				wfq.accept(dbTarget)
			if self.isExitRequested(): return False

		return True
#----------------------------------------------------------------------------------------------------


#----------------------------------------------------------------------------------------------------
#----------------------------------------------------------------------------------------------------




#----------------------------------------------------------------------------------------------------
app = WfqQuery(len(sys.argv), sys.argv)
sys.exit(app())
#----------------------------------------------------------------------------------------------------

