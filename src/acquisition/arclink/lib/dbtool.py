#***************************************************************************** 
# dbtool.py
#
#  reads/writes the inventory, routing and/or access entries from an
#  'old-style' SC database, Seiscomp3 database or Arclink server
#
#
# (c) 2007 Mathias Hoffmann, GFZ Potsdam
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any later
# version. For more information, see http://www.gnu.org/
#*****************************************************************************

import sqlobject
from seiscomp.db.sqlobject.inventory import Inventory as SC_Inventory
from seiscomp.db.sqlobject.routing import Routing as SC_Routing

from seiscomp3 import IO, DataModel, Logging, Core, Communication
from seiscomp.db.seiscomp3.inventory import Inventory as SC3_Inventory
from seiscomp.db.seiscomp3.routing import Routing as SC3_Routing
import seiscomp.db.seiscomp3.sc3wrap as sc3wrap

from seiscomp.arclink.manager import *

from optparse import OptionParser
import datetime, time
import sys, os, shutil


VERSION = "0.1 (2008.50)"


class dbtools(object):

	def __init__(self):
		pass

	def open_scDb(self, dbUrl):
		m = re.match("(?P<dbDriverName>^.*):\/\/(?P<dbAddress>.+?:.+?@.+?\/.+$)", dbUrl)
		if not m:
			raise SystemExit, "error in parsing local_db_url"
		_dbUrl = m.groupdict()
		
		print "opening source Database: " + _dbUrl["dbAddress"]
		connection = sqlobject.connectionForURI(_dbUrl["dbDriverName"]+"://"+_dbUrl["dbAddress"])
		sqlobject.sqlhub.processConnection = connection
	
	
	def open_SC3MsgDb(self, msgUrl, dbUrl):
		m = re.match("(?P<userName>^.*)@(?P<msgAddress>.+?:.+$)", msgUrl)
		if not m:
			raise SystemExit, "error in parsing SC3 msg url"
		_msgUrl = m.groupdict()
		m = re.match("(?P<dbDriverName>^.*):\/\/(?P<dbAddress>.+?:.+?@.+?\/.+$)", dbUrl)
		if not m:
			raise SystemExit, "error in parsing SC3 DB url"
		_dbUrl = m.groupdict()
		
		dbDriver = IO.DatabaseInterface.Create(_dbUrl["dbDriverName"])
		if dbDriver is None:
			Logging.error("Cannot find database driver " + _dbUrl["dbDriverName"])
			raise SystemExit, "driver not found"
		if not dbDriver.connect(_dbUrl["dbAddress"]):
			Logging.error("Cannot connect to database at " + _dbUrl["dbAddress"])
			raise SystemExit, "connection could not be established"
		print "opening destination Database: " + _dbUrl["dbAddress"]
		dbQuery = DataModel.DatabaseQuery(dbDriver)
		sc3wrap.dbQuery = dbQuery
		
		DataModel.Notifier.Enable()
		print "connecting to Mediator: " + _msgUrl["msgAddress"] + " as " + _msgUrl["userName"]
		msgConnection = Communication.Connection.Create(_msgUrl["msgAddress"], _msgUrl["userName"], "INVENTORY")
		if msgConnection is None:
			Logging.error("Cannot connect to Mediator")
			raise SystemExit, "connection to Mediator could not be established"
		else:
			Logging.info("Connection has been established")
		
		return (msgConnection, dbQuery)
	
	
	def open_arclManager(self, arcUrl):
		m = re.match("(?P<userName>^.*)@(?P<arclAddress>.+?:.+$)", arcUrl)
		if not m:
			raise SystemExit, "error in parsing arclink url"
		_arcUrl = m.groupdict()
		
		return ArclinkManager(_arcUrl["arclAddress"], _arcUrl["userName"])
	
	
	
	def read_arclink(self, arclinkManager, inventory_xml, ma=None):
		print "loading inventory from remote Arclink server ..."
		arcInv = arclinkManager.get_inventory("*", "*", "*", "*", instr=True, allnet=True, modified_after=ma)
		arcInv.save_xml(inventory_xml, instr=2)
		print "saving to file: ", inventory_xml
	
	def read_scDb(self, inventory_xml, routing_xml, inv=False, rtn=False, acc=False, ma=None):
		if inv:
			print "loading inventory from SC DB ..."
			inv = SC_Inventory()
			inv.load_stations("*", modified_after=ma)
			inv.load_stations("*", "*", modified_after=ma)
			inv.load_stations("*", "*", "*", modified_after=ma)
			inv.load_instruments(modified_after=ma)
			inv.save_xml(inventory_xml, instr=2)
			print "saving to file: ", inventory_xml
		
		if rtn or acc:
			routing = SC_Routing()
			if rtn:
				print "loading routes from SC DB ..."
				routing.load_routes(modified_after=ma)
			if acc:
				print "loading access from SC DB ..."
				routing.load_access(modified_after=ma)
			routing.save_xml(routing_xml, use_access=True)
			print "saving to file: ", routing_xml
	
	def read_sc3Db(self, connection, inventory_xml, routing_xml, inv=False, rtn=False, acc=False, ma=None):
		(msgConnection, dbQuery) = connection
		if inv:
			print "loading inventory from SC3 DB ..."
			inv = SC3_Inventory(dbQuery.loadInventory())
			inv.load_stations("*", modified_after=ma)
			inv.load_stations("*", "*", modified_after=ma)
			inv.load_stations("*", "*", "*", modified_after=ma)
			inv.load_stations("*", "*", "*", "*", modified_after=ma)
			inv.load_instruments(modified_after=ma)
			inv.save_xml(inventory_xml, instr=2)
			print "saving to file: ", inventory_xml
			
		if rtn or acc:
			routing = SC3_Routing(dbQuery.loadRouting())
			if rtn:
				print "loading routes from dest DB ..."
				routing.load_routes(modified_after=ma)
			if acc:
				print "loading access from dest DB ..."
				routing.load_access(modified_after=ma)
			routing.save_xml(routing_xml, use_access=True)
			print "saving to file: ", routing_xml
	
	
	def write_scDb(self, inventory_xml, routing_xml, inv=False, rtn=False, acc=False):
		if inv:
			print "loading inventory from SC DB ..."
			inventory = SC_Inventory()
			inventory.load_stations("*")
			inventory.load_stations("*", "*")
			inventory.load_stations("*", "*", "*")
			inventory.load_instruments()
			print "merging inventory ..."
			try:
				inventory.load_xml(inventory_xml)
			except IOError:
				print "warning: can't read inventory file:", inventory_xml
		
		if rtn or acc:
			routing = SC_Routing()
			if rtn:
				print "loading routes from SC DB ..."
				routing.load_routes()
			if acc:
				print "loading access from SC DB ..."
				routing.load_access()
			print "merging routes/access ..."
			try:	
				routing.load_xml(routing_xml, use_access=True)
			except IOError:
				print "warning: can't read routing file:", routing_xml
	
	
	def write_sc3Db(self, connection, inventory_xml, routing_xml, inv=False, rtn=False, acc=False):
		(msgConnection, dbQuery) = connection
		
		if inv:
			print "loading inventory from SC3 DB ..."
			inventory = SC3_Inventory(dbQuery.loadInventory())
			inventory.load_stations("*")
			inventory.load_stations("*", "*")
			inventory.load_stations("*", "*", "*")
			inventory.load_stations("*", "*", "*", "*")
			inventory.load_instruments()
		
			print "merging inventory ..." 
			try:
				inventory.load_xml(inventory_xml)
			except IOError:
				print "warning: can't read inventory file:", inventory_xml
			inventory.flush()
		
		
		if rtn or acc:
			routing = SC3_Routing(dbQuery.loadRouting())
			routing.load_routes()
			routing.load_access()
			print "merging routes/access ..." 
			try:
				routing.load_xml(routing_xml, use_access=acc)
			except IOError:
				print "warning: can't read routing file:", routing_xml
			routing.flush()
		
		Nsize = DataModel.Notifier.Size()
		if Nsize > 0:
			print "trying to apply %d changes..." % Nsize
		else:
			print "no changes. ok."
			sys.exit(0)
		Nmsg = DataModel.Notifier.GetMessage(True)
		it = Nmsg.iter()
		msg = DataModel.NotifierMessage()
		maxmsg = 100
		sent = 0
		mcount = 0
		try:
			while it.get():
				msg.attach(DataModel.Notifier_Cast(it.get()))
				mcount += 1
				if msg and mcount == maxmsg:
					sent += mcount
					print "sending message (%5.1f %%)" % (sent / float(Nsize) * 100.0)
					msgConnection.send("INVENTORY", msg)
					msg.clear()
					mcount = 0
				it.next()
		except:
			pass
		finally:
			if msg:
				print "sending message (%5.1f %%)" % 100.0
				msgConnection.send("INVENTORY", msg)
				msg.clear()
	
	
	
	def remove_file(self, filename):
		try:
			os.remove(filename)
		except:
			pass
	

# <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	
	
def init():
	parser = OptionParser(usage="usage: %prog {--read (arclink|scDb|sc3Db) | --write (scDb|sc3Db)} {-i -r -a} {-A ARCLINKURL | -d scDbURL | -D scDb3URL -M SC3MSGURL }\n\
	                    {-I INVENTORYXML | -R ROUTINGXML}\n\n\
examples:\n\n\
python %prog --read arclink -i -A sysop@localhost:18001 -m 2007-12-31 -I inventory.xml\n\
--> reads the inventory entries - which are more recent than 2007-12-31 - from an Arclink server and saves them in a xml file\n\n\
python %prog --read scDb -i -d mysql://username:password@localhost/seiscomp -I inventory.xml\n\
--> reads the inventory entries from a scDb and saves them in a xml file\n\n\
python %prog --write sc3Db -i -D mysql://username:password@localhost/seiscomp3 -M username@localhost:4803 -I inventory.xml\n\
--> writes the inventory entries from a xml file and saves them to the sc3Db\n\n\
python %prog --read scDb -ira -d mysql://username:password@localhost/seiscomp -I inventory.xml -R routing.xml\n\
--> reads the inventory and routing/access entries from a scDb and saves them in xml files\n\n\
",
	version="%prog v" + VERSION)
	
	parser.add_option("", "--read", action="store", type="string", dest="source", default="", help="specify source: arclink|scDb|sc3Db")
	parser.add_option("", "--write", action="store", type="string", dest="target", default="", help="specify target: scDb|sc3Db")
	
	parser.add_option("-i", "--inventory", action="store_true", dest="inventory", default=False, help="load inventory")
	parser.add_option("-r", "--routing", action="store_true", dest="routing", default=False, help="load routing")
	parser.add_option("-a", "--access", action="store_true", dest="access", default=False, help="load access")
	
	parser.add_option("-d", "--scDbURL", action="store", type="string", dest="scDbURL", default="", help="URL of Seiscomp Database e.g.: mysql://username:password@hostname/seiscomp")
	parser.add_option("-D", "--sc3DbURL", action="store", type="string", dest="sc3DbURL", default="", help="URL of Seiscomp3 Database e.g.: mysql://username:password@hostname/seiscomp3")
	parser.add_option("-M", "--sc3MsgURL", action="store", type="string", dest="sc3MsgURL", default="", help="URL of Seiscomp3 Mediator e.g.: username@hostname:4803")
	parser.add_option("-A", "--arclinkURL", action="store", type="string", dest="arclinkURL", default="", help="URL of Arclink Server")
	
	parser.add_option("-I", "--inventoryXML", action="store", type="string", dest="inventoryXML", default="", help="Inventory XML File [for options: -i]")
	parser.add_option("-R", "--routingXML", action="store", type="string", dest="routingXML", default="", help="Routing XML File [for options: -r or -a]")
		
	parser.add_option("-m", "--modifiedAfter", action="store", type="string", dest="modifiedAfter", default="", help="use only data modified after e.g.                 \"YYYY-MM-DD[ hh:mm:ss]\" [None]")
	
	(options, args) = parser.parse_args()
	
	
	if (options.source != "" and options.target == ""):
		m = re.match(r'^(arclink|scDb|sc3Db)$', options.source)
		if m:
			if m.string == "arclink" and options.arclinkURL == '':
				parser.error("missing arclink URL!\n")
				
			if m.string == "scDb" and options.scDbURL == '':
				parser.error("missing scDb URL!\n")
				
			if m.string == "sc3Db" and (options.sc3DbURL == '' or options.sc3MsgURL == ''):
				parser.error("missing sc3Db or sc3Msg URL!\n")
		else:
			parser.error("invalid source!\n\nSee -h for a list of options.")
	else:
		if (options.source == "" and options.target != ""):
			m = re.match(r'^(scDb|sc3Db|file)$', options.target)
			if m:
				if m.string == "scDb" and options.scDbURL == '':
					parser.error("missing scDb URL!\n")
					
				if m.string == "sc3Db" and (options.sc3DbURL == '' or options.sc3MsgURL == ''):
					parser.error("missing sc3Db or sc3Msg URL!\n")
			else:
				parser.error("invalid target!\n\nSee -h for a list of options.")
		else:
			parser.error("Please use either --read or --write\n\nSee -h for a list of options.")
	
	if not options.inventory and not options.routing and not options.access:
		parser.error("Please set at least: -i -r or -a\n")
	
	if options.inventory and options.inventoryXML == "":
		parser.error("no InventoryXML Filename given: -I file\n")
	
	if (options.routing or options.access) and options.routingXML == "":
		parser.error("no RoutingXML Filename given: -R file\n")
	
	if options.modifiedAfter != "":
		t = options.modifiedAfter
		try:
			options.modifiedAfter = datetime.datetime.strptime(t, "%Y-%m-%d")
		except ValueError:
			try:
				options.modifiedAfter = datetime.datetime.strptime(t, "%Y-%m-%d %H:%M:%S")
			except ValueError:
				parser.error("wrong time format: %s -> modifiedAfter ignored!\n" % t)
	else:
		options.modifiedAfter = None
	
	#if options.inventoryXML != "":
		#statinfo = os.stat(options.inventoryXML)
			
			
	return (options, args)

# <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


(options, args) = init()

dbt = dbtools()

# --read
if options.source == 'arclink':
	arclinkManager = dbt.open_arclManager(options.arclinkURL)
	dbt.read_arclink(arclinkManager, options.inventoryXML, ma=options.modifiedAfter)

if options.source == 'scDb':
	dbt.open_scDb(options.scDbURL)
	dbt.read_scDb(options.inventoryXML, options.routingXML, inv=options.inventory, rtn=options.routing, acc=options.access, ma=options.modifiedAfter)

if options.source == 'sc3Db':
	readConnection = dbt.open_SC3MsgDb(options.sc3MsgURL, options.sc3DbURL)
	dbt.read_sc3Db(readConnection, options.inventoryXML, options.routingXML, inv=options.inventory, rtn=options.routing, acc=options.access, ma=options.modifiedAfter)


# --write
if options.target == 'scDb':
	dbt.open_scDb(options.scDbURL)
	dbt.write_scDb(options.inventoryXML, options.routingXML, inv=options.inventory, rtn=options.routing, acc=options.access)

if options.target == 'sc3Db':
	writeConnection = dbt.open_SC3MsgDb(options.sc3MsgURL, options.sc3DbURL)
	dbt.write_sc3Db(writeConnection, options.inventoryXML, options.routingXML, inv=options.inventory, rtn=options.routing, acc=options.access)















