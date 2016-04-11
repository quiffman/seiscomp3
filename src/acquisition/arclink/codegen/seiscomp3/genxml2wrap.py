#!/usr/bin/env python

## ***************************************************************************** 
## genxml2wrap.py: code generator
##
## generates arclink inventory and routing wrapper python code

## read tasks from ./defintion.py
## input: generic.xml (Seiscomp3 schema)
## output: xyzwrap.py (Seiscomp3 object wrapper code for arclink)
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

import sys
import os
from Cheetah.Template import Template
from data import *
from query import *
from xmlreader import Reader

import definition

versionMajor = int(sys.version[0])
versionMinor = int(sys.version[2])
if versionMajor < 2 or (versionMajor == 2 and versionMinor < 3):
    print "You need at least Python 2.3 to run this script"
    print "Found version %d.%d" % (versionMajor, versionMinor)
    sys.exit(1)

baseDirectory = os.path.dirname(sys.argv[0])
if not baseDirectory:
    baseDirectory = "."
print "using basedirectory: %s" % baseDirectory

elementCount = 0
schemeXML = baseDirectory + "/generic.xml"
scheme = None
config = Config()

reader = Reader(schemeXML)
reader.config = config
scheme = reader.readScheme(scheme)
elementCount += reader.elementCount

if scheme is None:
    print "error loading scheme..."
    sys.exit(0)

print "scheme '%s' loaded..." % scheme.name
print "%d elements fetched..." % elementCount

for task in definition.taskList:
	f = open(task.target, "w")
	f.write(task.header)
	for element in scheme.getTypes():
		rootElement = element.getRootElement()
		if (rootElement):
			if (rootElement.Name() in task.packages):
				t = Template(file = task.template)
				t.config = config
				t.classInfo = element
				t.rootElement = rootElement.Name()
				f.write(str(t))
	f.write(task.footer)
	f.close()
	print "created '%s'..." % task.target


sys.exit(0)
