############################################################################
#    Copyright (C) 2006 by Jan Becker
#    geofon_devel@gfz-potsdam.de
#
#    This program is free software; you can redistribute it and#or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the
#    Free Software Foundation, Inc.,
#    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
############################################################################

import data, query

try:
	# Python 2.5
	from xml.etree import ElementTree
	from xml.etree.ElementTree import Element

except ImportError:
	from elementtree import ElementTree
	from elementtree.ElementTree import Element


def tagname(element):
	names = element.tag.split("}")
	if len(names) == 0:
		return ""
	
	return names.pop()


class Reader:
	def __init__(self, filename):
		self.filename = filename
		self.tree = None
		self.element = None
		self.elementCount = 0
		self.scheme = None
		self.config = None
		self.types = []

	def collectOptions(self, enum, node):
		for child in node:
			if tagname(child) == 'option':
				name = child.get('name')
				value = child.get('value')
				if not name or not value:
					print "WARNING: option without name or value in enum '%s' found => ignoring" % enum.name
				else:
					enum.addOption(data.EnumOption(name, value))

	def readEnum(self, node):
		name = node.get('name')

		if name:
			enum = data.Enum(name)
			self.collectOptions(enum, node)
			return enum

		if not name:
			print "WARNING: enum without name found => ignoring"

		return None

	def readAttribute(self, node):
		name = node.get('name')
		type = node.get('type')
		typeRef = node.get('typeref')

		optional = True
		optionalAttrib = node.get('optional')
		if optionalAttrib:
			optional = (optionalAttrib == "true")
		#optional = False

		if name and (type or typeRef):
			if type:
				if type == "string":
					optional = False
				attrib = data.Attribute(name, type, optional)
				
				attrib.sqlType = node.get('sqltype')
				attrib.sqlLength = node.get('length')
				attrib.sqlPrecision = node.get('precision')
				if node.get('sqlindex'):
					attrib.sqlIndex = node.get('sqlindex') == "true"
				else:
					attrib.sqlIndex = False
			else:
				schemeType = self.scheme.getType(typeRef) or self.scheme.getEnum(typeRef)
				if not schemeType:
					print "WARNING: type '%s' does not exist used for attribute '%s'" % (typeRef, name)
				attrib = data.Attribute(name, typeRef, optional)
				if node.get('sqltype'):
					print "WARNING: sqltype for typeref'd attributes are not allowed -> ignoring"
			
			if attrib.sqlType and attrib.sqlType == "table" and schemeType:
				#attrib.sqlType = None
				schemeType.setOwnTable(True)

			return attrib

		if not name:
			print "WARNING: attribute without name in element '%s' found => ignoring" % self.element.name

		if not type:
			print "WARNING: attribute '%s' without type in element '%s' found => ignoring" % (name, self.element.name)

		return None


	def readElement(self, node):
		element = None
		name = node.get('name')
		table = node.get('table')
		#type = node.get('typeref')

		#if not type:
		#	print "ERROR: found element without type"
		#	return None

		if name:
			if table:
				element = data.Class(table)
			else:
				element = data.Class(name)
			element.rawname = table
		else:
			return None

		self.element = element

		indexNode = None
		
		# collect attributes
		for child in node:
			if tagname(child) == 'attribute':
				attrib = self.readAttribute(child)
				if attrib != None:
					if attrib.name == 'publicID':
						element.setPublic()
						element.addInheritedAttribute(data.Attribute(self.config.publicIdAttrib, self.config.publicIdType, False))
					else:
						xmlType = child.get('xmltype')
						sqlType = attrib.sqlType
						archiveHint = None
						if xmlType:
							if xmlType == 'element':
								archiveHint = "Archive::XML_ELEMENT"
							if xmlType == 'cdata':
								archiveHint = "Archive::XML_CDATA"
	
						if sqlType:
							if sqlType == 'table':
								if archiveHint:
									archiveHint = archiveHint + ' | Archive::DB_TABLE'
								else:
									archiveHint = 'Archive::DB_TABLE'
							elif sqlType == 'DATETIME+INT':
								if archiveHint:
									archiveHint = archiveHint + ' | Archive::SPLIT_TIME'
								else:
									archiveHint = 'Archive::SPLIT_TIME'
	
						element.addAttribute(attrib, archiveHint)
						if xmlType and xmlType == 'cdata':
							element.setCData(attrib)

						if child.get('primary') and child.get('primary') == "true":
							element.setPrimaryAttribute(attrib)

			elif tagname(child) == 'index':
				indexNode = child
			elif tagname(child) == 'element':
				childName = child.get('name')
				childType = child.get('typeref')
				if not childName or not childType:
					print "ERROR: 'name' and 'typeref' are required when defining elements"
				else:
					child = self.scheme.getType(childType)
					if not child:
						print "ERROR: type '%s' is not defined used as element in type '%s'" % (childType, name)
					elif element.isPublicType():
						element.addElement(child, childName)
					else:
						print "ERROR: only elements with attribute 'publicID' can have childs"

		if indexNode != None:
			unique = indexNode.get('unique')
			if element.addIndex(indexNode.get('name'), unique == "true"):
				attribs = indexNode.get('expr').replace(' ', '').split(',')
				for attrib in attribs:
					element.addIndexAttribute(attrib)
		#else:
			#if element.addIndex("Idx" + element.name, True):
				#for attrib in element.getAttributes():
					#element.addIndexAttribute(attrib.name)

		#self.collectElements(element, node)

		for attrib in element.getAttributes():
			if element.isIndexAttribute(attrib):
				attrib._optional = False
		
		return element


	def readType(self, node):
		type = None
		name = node.get('name')
		if name:
			type = data.Type(name)
			type.default = None
		else:
			return None
		
		# collect attributes
		for child in node:
			if tagname(child) == 'attribute':
				attrib = self.readAttribute(child)
				if attrib:
					xmlType = child.get('xmltype')
					archiveHint = None
					if xmlType and xmlType == 'element':
						archiveHint = "Archive::XML_ELEMENT"
						print "added Archive::XML_ELEMENT for attribute %s in %s" % (attrib.name, type.name)

					type.addAttribute(attrib, archiveHint)
					xmlType = child.get('xmltype')
					if xmlType and xmlType == 'cdata':
						type.setCData(attrib)

		#self.collectElements(type, node)
		
		return type


	def collectTypes(self, container, node):
		for child in node:
			if tagname(child) == 'type':
				element = self.readElement(child)
				if element != None:
					self.elementCount += 1
					self.scheme.addType(element)
			#elif child.tag == 'type':
			#	type = self.readType(child)
			#	if type:
			#		container.addType(type)
			elif tagname(child) == 'enum':
				enum = self.readEnum(child)
				if enum:
					if isinstance(container, data.Scheme):
						container.addEnum(enum)
					else:
						print "WARNING: enum '%s' is no top level element" % enum.name
			elif tagname(child) == 'element':
				childName = child.get('name')
				childType = child.get('typeref')
				if not childName or not childType:
					print "ERROR: 'name' and 'typeref' are required when defining elements"
				else:
					child = self.scheme.getType(childType)
					if not child:
						print "ERROR: type '%s' is not defined used as root element" % childType
					else:
						print "INFO: added root element '%s'" % childType
						child.setRootElement(True)
						container.addElement(child)


	def readElementAttribute(self, node):
		element = node.get('element')
		attrib = node.get('attribute')
		type = node.get('type')

		elementAttrib = None

		if element != None:
			instance = self.scheme.getElementOrType(element)
			if instance is None:
				print "ERROR: element '%s' is not a valid element" % element
				return None

			if attrib:
				if attrib != "*":
					attribute = instance.getAttribute(attrib)
					if attribute is None:
						print "ERROR: element '%s' has no attribute '%s'" % (element, attrib)
						return None
					attributeType = self.scheme.getType(attribute.type)
					if attributeType and len(attributeType.getAttributes()) > 1:
						primaryAttribute = attributeType.getPrimaryAttribute()
						if not primaryAttribute:
							print "ERROR: type '%s' of attribute '%s' of element '%s' has no primary attribute" % (attributeType.name, attrib, element)
							return None
			
			elementAttrib = query.ElementAttribute(instance, attrib)
		
		elif type != None:
			elementAttrib = query.ElementAttribute(type, None)
		
		return elementAttrib

	def readParameter(self, node):
		name = node.get('name')
		if name is None:
			print "ERROR: a parameter must have a name"
			return None
		
		element = node.get('element')
		attrib = node.get('attribute')
		type = node.get('type')

		elementAttrib = self.readElementAttribute(node)
		if elementAttrib is None:
			print "ERROR: parameter '%s' must have either an element,attribute pair or a type" % name
			return None

		if elementAttrib.boundToElement() and not elementAttrib.boundToAttribute():
			print "ERROR: parameter '%s' must be bound to an elementattribute" % name
			return None

		param = query.Parameter(name, elementAttrib)
		return param


	def readJoin(self, node):
		join = query.Join()
		for child in node:
			if tagname(child) == 'operand':
				elementAttribute = self.readElementAttribute(child)
				if elementAttribute != None:
					if elementAttribute.boundToAttribute():
						join.addOperand(elementAttribute)
					else:
						print "ERROR: a join operand must reference an elementattribute"
						return None
				else:
					print "ERROR: while reading the join -> skipping"
					return None

		return join


	def readCondition(self, q, node):
		operation = node.get('operation')
		if operation is None:
			print "ERROR: condition for parameter '%s' must define an operation" % node.get('param')
			return None

		param = q.getParam(node.get('param'))
		if param is None and operation != "is_unset" and operation != "is_set":
			print "ERROR: the condition must reference an existing parameter"
			return None
		
		elementAttribute = self.readElementAttribute(node)
		if elementAttribute:
			if not elementAttribute.boundToAttribute():
				print "ERROR: a condition must reference an elementattribute"
				return None
			#if param and param.target.boundToElement():
			#	print "ERROR: the condition parameter '%s' elementattribute was already defined by parameter tag" % node.get('param')
			#	return None
		else:
			if param:
				if not param.target.boundToAttribute():
					print "ERROR: the condition parameter '%s' must reference an elementattribute" % node.get('param')
					return None
			else:
				print "ERROR: the condition must reference an existing parameter"
				return None

		cond = query.Condition(node.get('param'), operation, elementAttribute)
		return cond


	def readConditionGroup(self, q, node):
		operation = node.get('operation')
		if operation is None:
			print "ERROR: a conditiongroup must define an operation"
			return None
		
		group = query.ConditionGroup(operation)
		
		for child in node:
			if tagname(child) == 'condition':
				cond = self.readCondition(q, child)
				if cond != None:
					group.addCondition(cond)
				else:
					print "ERROR: while reading conditiongroup -> skipping"
					return None

			elif tagname(child) == 'conditiongroup':
				childGroup = self.readConditionGroup(q, child)
				if childGroup is None:
					return None

				group.addCondition(childGroup)

		return group


	def addHierarchieJoin(self, q, parentType, childType):
		if q.hasSource(parentType) and q.hasSource(childType):
			join = query.Join()
			#refAttrib = parentType.name
			refAttrib = "parent"
			join.addOperand(query.ElementAttribute(childType, "_" + refAttrib + "_oid"))
			join.addOperand(query.ElementAttribute(parentType, "_oid"))
			q.addJoin(join)
			print "INFO: add hierarchie join between %s and %s" % (parentType.name, childType.name)


	def readQuery(self, node):
		q = query.Query(node.get('name'))

		for child in node:
			if tagname(child) == 'documentation':
				q.documentation = child.text
			if tagname(child) == 'parameter':
				param = self.readParameter(child)
				if param is None:
					print "ERROR: while reading the query '%s' -> skipping" % node.get('name')
					return None

				if not q.addParam(param):
					return None

			elif tagname(child) == 'source':
				if not q.addSource(self.scheme.getElementOrType(child.get('element'))):
					return None

			elif tagname(child) == 'join':
				join = self.readJoin(child)
				if join is None:
					print "ERROR: while reading the query '%s' -> skipping" % node.get('name')
					return None

				if not q.addJoin(join):
					return None

			elif tagname(child) == 'result':
				elementAttrib = self.readElementAttribute(child)
				if elementAttrib != None:
					if not elementAttrib.boundToElement():
						print "ERROR: query '%s' result must be bound to an element or an elementattribute" % q.name
						return None

					groupAttrib = child.get('groupBy')
					if groupAttrib:
						attribute = elementAttrib.type.getAttribute(groupAttrib)
						if attribute is None:
							print "ERROR: element '%s' has no attribute '%s'" % (elementAttrib.type.name, groupAttrib)
							return None

					if not q.hasSource(elementAttrib.type):
						q.addSource(elementAttrib.type)

					q.setResult(elementAttrib, groupAttrib)
				else:
					print "ERROR: query '%s' has no defined result" % q.name
					return None

			elif tagname(child) == 'condition':
				cond = self.readCondition(q, child)
				if cond is None:
					print "ERROR: while reading the query '%s' -> skipping" % node.get('name')
					return None
				
				q.addCondition(cond)

			elif tagname(child) == 'conditiongroup':
				group = self.readConditionGroup(q, child)
				if group is None:
					print "ERROR: while reading the query '%s' -> skipping" % node.get('name')
					return None

				q.addCondition(group)

		if not q.hasResult():
			print "ERROR: query '%s' has no defined result" % q.name
			return None

		print "INFO: building hierarchy joins for " + q.name + " and " + ",".join([i.name for i in q.sources])
		# Build joins between parent/child tables
		for source in q.sources:
			element = self.scheme.getElementOrType(source.name)
			if isinstance(element, data.Class):
				for child in element.getElements():
					self.addHierarchieJoin(q, element, child)

		# Add default conditions for parameters bound to elementattributes
		for paramName in q.parameters:
			param = q.getParam(paramName)
			if param and param.target.boundToAttribute():
				if not q.hasConditionFor(param):
					q.addCondition(query.Condition(paramName, "eq", param.target))

		return q


	def collectQueries(self, queries, node):
		for child in node:
			if tagname(child) == 'query':
				query = self.readQuery(child)
				if query != None:
					queries.append(query)


	def readScheme(self, scheme):
		try:
			self.tree = ElementTree.parse(self.filename)
		except:
			return None

		if self.tree is None:
			return scheme

		root = self.tree.getroot()
		if root is None:
			return None

		if tagname(root) == 'seiscomp-schema':
			if scheme is None:
				self.scheme = data.Scheme(tagname(root))
			self.collectTypes(self.scheme, root)
		else:
			print "ERROR: wrong root tag '%s'" % root.tag
		
		return self.scheme

	def readQueries(self, scheme, queries):
		try:
			self.tree = ElementTree.parse(self.filename)
		except:
			return None

		if self.tree is None:
			return queries

		if scheme is None:
			print "ERROR: read queries: no scheme given"
			return None
		
		self.scheme = scheme

		root = self.tree.getroot()
		if root is None:
			return None

		if tagname(root) == 'seiscomp-schema-queries':
			if queries is None:
				queries = []

			self.collectQueries(queries, root)

		return queries
