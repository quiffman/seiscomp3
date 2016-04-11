## ***************************************************************************** 
## definition.py: code generator definition
##
## (c) 2010 Mathias Hoffmann, GFZ Potsdam
##
## This program is free software; you can redistribute it and/or modify it
## under the terms of the GNU General Public License as published by the
## Free Software Foundation; either version 2, or (at your option) any later
## version. For more information, see http://www.gnu.org/
## *****************************************************************************
##
## 2010.014
##

##################################################################################
class Task:
	target = ""
	header = """\
# This file was created by a source code generator:
# genxml2wrap.py 
# Do not modify. Change the definition and
# run the generator again!
#
# (c) 2010 Mathias Hoffmann, GFZ Potsdam
#
#
"""
	packages = ""
	template = ""
	footer = ""

taskList = list()
##################################################################################




##################################################################################
task = Task()
task.target = "../../../../environment/lib/python/seiscomp/db/seiscomp3/sc3wrap.py"
task.header += """\
import re
import time
import datetime
from seiscomp import logs
from seiscomp3 import DataModel, Core, Config

#(de)serialize real array string
def str2RealArray(arg):
	RA = DataModel.RealArray()
	if not arg:
		return RA
	sarg = arg.split()
	for s in sarg:
		if len(s):
			RA.content().push_back(float(s))
	return RA
def RealArray2str(arg):
	tmp = str()
	for r in arg:
		tmp += str(r)+' '
	return tmp.strip()

#de-serialize complex array string
def str2ComplexArray(arg):
	CA = DataModel.ComplexArray()
	if not arg:
		return CA
	sarg = re.findall(r'\((.+?)\)', arg)
	for s in sarg:
		r,i = s.split(',')
		if len(r) and len(i):
			CA.content().push_back(complex(float(r), float(i)))
	return CA
def ComplexArray2str(arg):
	tmp = str()
	for c in arg:
		tmp += '('+str(c.real)+','+str(c.imag)+') '
	return tmp.strip()
#
#


"""

task.packages =  ['QualityControl', 'Inventory', 'Routing']
task.template = "sc3wrap.epy"
task.footer = ""
##################################################################################
taskList.append(task)



##################################################################################
task = Task()
task.target = "../../../../environment/lib/python/seiscomp/db/generic/genwrap.py"
task.header += """\
import datetime

class _TrackedObject(object):
	def __init__(self):
		self.__dict__['last_modified'] = datetime.datetime(1970, 1, 1, 0, 0, 0)

	def __setattr__(self, name, value):
		if name not in self.__dict__ or self.__dict__[name] != value:
			self.__dict__[name] = value
			self.__dict__['last_modified'] = datetime.datetime.utcnow()
#
#


"""

task.packages =  ['QualityControl', 'Inventory', 'Routing']
task.template = "genwrap.epy"
task.footer = ""
##################################################################################
taskList.append(task)



##################################################################################
task = Task()
task.target = "../../../../environment/lib/python/seiscomp/db/xmlio/xmlwrap.py"
task.header += """\
import re
import datetime

def Property(func):
        return property(**func())

try:
	import xml.etree.cElementTree as ET  # Python 2.5?
except ImportError:
	import cElementTree as ET

try:
	from decimal import Decimal   # Python 2.4
	_have_decimal = True
except ImportError:
	_have_decimal = False         # Will use float for decimal, hope it works

def _string_fromxml(val):
	if val is None:
		return ""

	return val.encode("utf-8", "replace")

def _string_toxml(val):
	if val is None:
		return ""

	if isinstance(val, str):
		try:
			return val.decode("utf-8")
		except UnicodeDecodeError:
			return val.decode("iso-8859-1", "replace")

	return unicode(val)

def _int_fromxml(val):
	if val is None or val == "":
		return None

	return int(val)

_int_toxml = _string_toxml

def _float_fromxml(val):
	if val is None or val == "":
		return None

	return float(val)

_float_toxml = _string_toxml

if _have_decimal:
	def _decimal_fromxml(val):
		if val is None or val == "":
			return None

		return Decimal(val)
else:
	_decimal_fromxml = _float_fromxml

_decimal_toxml = _string_toxml

def _boolean_fromxml(val):
	if val == "True" or val == "true":
		return True

	return False

def _boolean_toxml(val):
	if val:
		return "true"

	return "false"

# DataModel.DEPLOYMENT == 0
# DataModel.ARRAY == 1
def _StationGroupType_toxml(val):
	if val == 0:
		return "DEPLOYMENT"
	else:
		return "ARRAY"

def _StationGroupType_fromxml(val):
	if val == "DEPLOYMENT":
		return 0
	else:
		return 1

_rx_datetime = re.compile("([0-9]{4})-([0-9]{2})-([0-9]{2})T" \
	"([0-9]{2}):([0-9]{2}):([0-9]{2}).([0-9]*)" \
	"(Z|([+-])([0-9]{2}):([0-9]{2}))?$")

_rx_date = re.compile("([0-9]*)-([0-9]*)-([0-9]*)" \
	"(Z|([+-])([0-9]{2}):([0-9]{2}))?$")

def _datetime_fromxml(val = ""):
	if val is None or val == "":
		return None
		
	m = _rx_datetime.match(val)
	if m == None:
		m = _rx_date.match(val)
		if m == None:
			raise ValueError, "invalid datetime: " + val

		(year, month, mday, tz, plusminus, tzhours, tzminutes) = m.groups()

		try:
			# ignore time zone
			obj = datetime.datetime(int(year), int(month), int(mday), 0, 0, 0)
		except ValueError:
			raise ValueError, "invalid datetime: " + val
	else:
		(year, month, mday, hour, min, sec, sfract,
			tz, plusminus, tzhours, tzminutes) = m.groups()

		try:
			obj = datetime.datetime(int(year), int(month), int(mday),
				int(hour), int(min), int(sec), int((sfract + "000000")[:6]))
			if tz is not None and tz != "Z":
				delta = datetime.timedelta(hours = int(tzhours),
					minutes=int(tzminutes))
				if plusminus == "-":
					obj -= delta
				else:
					obj += delta
				
		except ValueError:
			raise ValueError, "invalid datetime: " + val

	return obj

def _datetime_toxml(val):
	if isinstance(val, datetime.datetime):
		return "%04d-%02d-%02dT%02d:%02d:%02d.%04dZ" % \
			(val.year, val.month, val.day, val.hour, val.minute,
			val.second, val.microsecond / 100)
	elif isinstance(val, datetime.date):
		return "%04d-%02d-%02d" % (val.year, val.month, val.day)

	elif val is None:
		return ""

	raise ValueError, "invalid date or datetime object"

def _get_blob(e, name):
	return _string_fromxml(e.findtext(name)).strip()
	
def _set_blob(e, name, value):
	text = _string_toxml(value).strip()
	e1 = ET.Element(name)
	e1.text = text
	e.append(e1)

def _get_quantity(e, name):
	e1 = e.find(name)
	if e1 is None:
		return quantity()

	return quantity(_float_fromxml(e1.text), unit=_string_fromxml(e1.get("unit")), error=_float_fromxml(e1.get("error")))

def _set_quantity(e, name, value):
	e1 = ET.Element(name)
	e1.text = _float_toxml(value)
	e1.set("unit", _string_toxml(value.unit))
	e1.set("error", _float_toxml(value.error))
	e.append(e1)
#
#


"""

task.packages =  ['QualityControl', 'Inventory', 'Routing']
task.template = "xmlwrap.epy"
task.footer = ""
##################################################################################
taskList.append(task)



##################################################################################
task = Task()
task.target = "../../../../environment/lib/python/seiscomp/db/generic/inventory.py"
task.header += """\
import genwrap as _genwrap
from seiscomp.db.xmlio import inventory as _xmlio
from seiscomp.db import DBError
#
#


"""

task.packages =  ['Inventory']
task.template = "genInventory.epy"
task.footer = """\

"""
##################################################################################
taskList.append(task)



##################################################################################
task = Task()
task.target = "../../../../environment/lib/python/seiscomp/db/generic/routing.py"
task.header += """\
import genwrap as _genwrap
from seiscomp.db.xmlio import routing as _xmlio
from seiscomp.db import DBError
#
#


"""

task.packages =  ['Routing']
task.template = "genInventory.epy"
task.footer = """\

"""
##################################################################################
taskList.append(task)



##################################################################################
task = Task()
task.target = "../../../../environment/lib/python/seiscomp/db/generic/qc.py"
task.header += """\
import genwrap as _genwrap
from seiscomp.db.xmlio import qc as _xmlio
from seiscomp.db import DBError
#
#


"""

task.packages =  ['QualityControl']
task.template = "genInventory.epy"
task.footer = """\

"""
##################################################################################
taskList.append(task)




















##################################################################################
task = Task()
task.target = "../../../../environment/lib/python/seiscomp/db/xmlio/inventory.py"
task.target = "./inventory.py"
task.header += """\
import xmlwrap as _xmlwrap
from seiscomp.db import DBError

try:
	from xml.etree import cElementTree as ET  # Python 2.5?
except ImportError:
	import cElementTree as ET

_root_tag = "{http://geofon.gfz-potsdam.de/ns/inventory/0.2/}inventory"
#
#


"""

task.packages =  ['Inventory']
task.template = "xmlInventory.epy"
task.footer = """\
#***************************************************************************** 
# Incremental Parser
#*****************************************************************************

class _IncrementalParser(object):
	def __init__(self, inventory):
		self.__inventory = inventory
		self.__p = ET.XMLTreeBuilder()

	def feed(self, s):
		self.__p.feed(s)

	def close(self):
		root = self.__p.close()
		if root.tag != _root_tag:
			raise DBError, "unrecognized root element: " + root.tag

		xinventory = _xmlwrap.xml_inventory(root)
		_xmldoc_in(xinventory, self.__inventory)
		

#***************************************************************************** 
# Public functions
#*****************************************************************************

def make_parser(inventory):
	return _IncrementalParser(inventory)

def xml_in(inventory, src):
	doc = ET.parse(src)
	root = doc.getroot()
	if root.tag != _root_tag:
		raise DBError, "unrecognized root element: " + root.tag

	xinventory = _xmlwrap.xml_inventory(root)
	_xmldoc_in(xinventory, inventory)

def _indent(elem, level=0):
	s = 1*"\t" # indent char
	i = "\n" + level*s
	if len(elem):
		if not elem.text or not elem.text.strip():
			elem.text = i + s
		for e in elem:
			_indent(e, level+1)
			if not e.tail or not e.tail.strip():
				e.tail = i + s
		if not e.tail or not e.tail.strip():
			e.tail = i
	else:
		if level and (not elem.tail or not elem.tail.strip()):
			elem.tail = i

def xml_out(inventory, dest, instr=0, modified_after=None, stylesheet=None, indent=True):
	xinventory = _xmlwrap.xml_inventory()
	
	_xmldoc_out(xinventory, inventory, instr, modified_after)

	if isinstance(dest, basestring):
		fd = file(dest, "w")
	elif hasattr(dest, "write"):
		fd = dest
	else:
		raise TypeError, "invalid file object"

	try:
		filename = fd.name
	except AttributeError:
		filename = '<???>'

	fd.write('<?xml version="1.0" encoding="utf-8"?>\n')

	if stylesheet != None:
		fd.write('<?xml-stylesheet type="application/xml" href="%s"?>\n' % \
			(stylesheet,))
	
	if indent is True:
		_indent(xinventory._element)
	
	ET.ElementTree(xinventory._element).write(fd, encoding="utf-8")
	
	if isinstance(dest, basestring):
		fd.close()

"""
##################################################################################
taskList.append(task)

