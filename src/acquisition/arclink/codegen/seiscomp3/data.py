# -*- coding: utf-8 -*-
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
from sets import Set as set
import re

def capitalizeFirst(s):
	if len(s) >= 2:
		return s[0].upper() + s[1:]
	elif len(s) == 1:
		return s[0].upper()

	return ""

def decapitalizeFirst(s):
	if len(s) >= 2:
		return s[0].lower() + s[1:]
	elif len(s) == 1:
		return s[0].lower()

	return ""



class TypeMapper:
	def __init__(self, name, default, include = '', primitive = True, own = False, type = None):
		self.name = name
		self.default = default
		self.primitive = primitive
		self.ownType = own
		self.include = include
		self.type = type


class Attribute:
	def __init__(self, name, type, optional):
		self.name = name
		self.type = type
		self._attribName = ""
		self._arrayType = False
		self._optional = optional
		self.sqlType = None
		self.sqlLength = None
		self.sqlPrecision = None
		self.sqlIndex = False

		if type.endswith('[]'):
			self._arrayType = True
			self.type = type[0:-2]

		words = self.name.split('_')
		cWords = []
		
		capitalize = False
		for word in words:
			if capitalize:
				cWords.append(capitalizeFirst(word))
			else:
				cWords.append(word)

			capitalize = True

		self.attribName = "".join(cWords)

	def Name(self):
		return self.attribName
	
	def oName(self):
		return self.name

	def CapitalizedName(self):
		return capitalizeFirst(self.attribName)

	def isArray(self):
		return self._arrayType

	def isOptional(self):
		return self._optional


class EnumOption:
	def __init__(self, name, value):
		self.name = name;
		self.value = value


class Enum:
	def __init__(self, name):
		self.name = name
		self.enumName = ""
		self._options = []
		
		words = self.name.split('_')
		cWords = []
		
		for word in words:
			cWords.append(capitalizeFirst(word))

		self.enumName = "".join(cWords)

	def Name(self):
		return self.enumName

	def addOption(self, option):
		if not option:
			return

		for opt in self._options:
			if opt.name == option.name or opt.value == option.value:
				return
			
		self._options.append(option)

	def getOptions(self):
		return self._options


class ElementContainer:
	def __init__(self):
		self._elements = []
		self._roles = dict()

	def addElement(self, element, role):
		if self in element.parents:
			print "ERROR: element '%s' has been already added to '%s'" % (element.name, self.name)
			return

		element.parents.append(self)
		self._elements.append(element)
		self._roles[element] = role

	def removeElement(self, element):
		if element in self._elements:
			self._elements.remove(element)
			del self._roles[element]

	def hasElements(self):
		return len(self._elements) > 0

	def countElements(self):
		return len(self._elements)

	def getElements(self):
		return self._elements

	def getElementRole(self, element):
		return self._roles[element]

	def getElementsFlat(self):
		ec = []
		for element in self.getElements():
			ec.append(element)
			ec += element.getElementsFlat()

		return ec

	def getElement(self, name):
		for element in self.getElements():
			if element.name == name:
				return element

			child = element.getElement(name)
			if child != None:
				return child

		return None


class Index:
	def __init__(self, name):
		self.name = name
		self.unique = True
		self._attributes = set()
		self._attributeList = []

	def addAttribute(self, attr):
		self._attributes.add(attr)
		self._attributeList.append(attr)

	def hasAttribute(self, attr):
		return attr in self._attributes

	def getAttributes(self):
		return self._attributeList

	def isEmpty(self):
		return len(self._attributes) == 0


class TypeBase(ElementContainer):
	def __init__(self, name):
		ElementContainer.__init__(self)
		self.name = name
		self._typeName = ""
		self._attributes = dict()
		self._attributeHints = dict()
		self._attributeList = []
		self._inheritedAttributes = []
		self._cdata = None
		self._primaryAttribute = None

		words = self.name.split('_')
		cWords = []
		
		for word in words:
			cWords.append(capitalizeFirst(word))

		self._typeName = "".join(cWords)

	def Name(self):
		return self._typeName

	def oName(self):
		return self.name

	def dependsOn(self, type):
		for attrib in self._attributeList:
			if attrib.type == type.name:
				return True
			
		for element in self.getElements():
			if element == type or element.dependsOn(type):
				return True
			
		return False

	def addInheritedAttribute(self, attr):
		self._inheritedAttributes.append(attr)
	
	def addAttribute(self, attr, archiveHint = None):
		if self._attributes.has_key(attr.name):
			print "ERROR: attribute '%s' already defined in '%s'" % (attr.name, self.name)
			return
			
		self._attributes[attr.name] = attr
		self._attributeHints[attr.name] = archiveHint
		self._attributeList.append(attr)

	def setPrimaryAttribute(self, attr):
		if self._primaryAttribute:
			raise AttributeError, "Primary attribute already defined with '%s'" % self._primaryAttribute.Name()

		self._primaryAttribute = attr

	def getPrimaryAttribute(self):
		return self._primaryAttribute

	def attributeHint(self, attr):
		if self._attributeHints.has_key(attr.name):
			return self._attributeHints[attr.name]
		return None

	def hasAttributes(self):
		return len(self._attributeList) > 0

	def hasNonIndexAttributes(self):
		count = 0
		for attr in self._attributeList:
			if not self.isIndexAttribute(attr):
				count = count + 1
		return count > 0

	def hasAttributesWithType(self, typename):
		for attrib in self._attributeList:
			if attrib.type == typename and not attrib.isArray():
				return True
		return False

	def getAttributes(self):
		return self._attributeList

	def getAttribute(self, name):
		if self._attributes.has_key(name):
			return self._attributes[name]

		for attr in self._inheritedAttributes:
			if attr.name == name:
				return attr

		return None

	def isIndexAttribute(self, attrib):
		return False
	
	def setCData(self, attr):
		if self._cdata:
			print "ERROR: cdata '%s' already defined in type '%s'" % (attr.name, self.name)
			return
			
		self._cdata = attr

	def cdata(self):
		return self._cdata

	def hasGetter(self):
		return True
	
	def getSQLAttributes(self, scheme, config, typeMapper, prefix="", defaultValues=False):
		attributes = []
		for attrib in self.getAttributes():
			sqlAttrib = ""
			type = scheme.getType(attrib.type)
			enum = scheme.getEnum(attrib.type)
			
			attribPrefix = attrib.name
			sqlIndex = attrib.sqlIndex
			if attribPrefix and prefix:
				attribPrefix = "_" + attribPrefix
			attribPrefix = prefix + attribPrefix

			if attrib.sqlType and attrib.sqlType != "table":
				sqlType = attrib.sqlType
				if sqlType == "DATETIME+INT":
					sqlType = "DATETIME"

				caseSensitive = False
				if sqlType.find("BINARY") != -1:
					sqlType = sqlType.replace(" BINARY", "").replace("BINARY","").strip()
					caseSensitive = True
				
				if typeMapper:
					sqlType = typeMapper.translate(sqlType, attrib.sqlLength, attrib.sqlPrecision, caseSensitive)
				elif attrib.sqlLength:
					sqlType = sqlType + "(" + attrib.sqlLength + ")"
				
				sqlAttrib = sqlAttrib + attribPrefix + " " + sqlType
					
				if attrib.sqlType == "DATETIME+INT":
					if not attrib.isOptional():
						sqlAttrib = sqlAttrib + " NOT NULL"
					attributes.append([sqlAttrib, attribPrefix, sqlIndex])
					attribPrefix = attribPrefix + "_ms"
					sqlAttrib = attribPrefix + " INTEGER"
			else:
				if type:
					if type.hasOwnTable():
						if typeMapper:
							sqlAttrib = sqlAttrib + attribPrefix + "_oid " + typeMapper.translate("INTEGER", 11, None, False)
						else:
							sqlAttrib = sqlAttrib + attribPrefix + "_oid INTEGER(11)"
					else:
						# TO DO - Add default value for child attributes that are not optional when the
						# attribute itself is not optional
						attributes += type.getSQLAttributes(scheme, config, typeMapper, attribPrefix, attrib.isOptional())
						if attrib.isOptional():
							if typeMapper:
								attributes.append([attribPrefix + "_used " + typeMapper.translate("BIT", None, None, False) + " NOT NULL DEFAULT 0", attribPrefix + "_used ", False])
							else:
								attributes.append([attribPrefix + "_used BIT NOT NULL DEFAULT 0", attribPrefix + "_used", False])
						continue
				elif enum:
					sqlAttrib = attribPrefix + " VARCHAR(64)"
				else:
					raise TypeError, 'No sqltype given'
					
			if not attrib.isOptional():
				sqlAttrib = sqlAttrib + " NOT NULL"
				if defaultValues and config.getTypeDefault(attrib.type):
					sqlAttrib = sqlAttrib + " DEFAULT " + config.getTypeDefault(attrib.type)

			attributes.append([sqlAttrib, attribPrefix, sqlIndex])
			
		return attributes

	def getSQLUniqueAttributes(self, scheme):
		uniqueAttribs = []
		for indexAttrib in self.getIndexAttributes():
			type = scheme.getType(indexAttrib.type)
			if type:
				for childAttrib in type.getAttributes():
					uniqueAttribs.append(indexAttrib.name + '_' + childAttrib.name)
			else:
				uniqueAttribs.append(indexAttrib.name)
				if indexAttrib.sqlType == "DATETIME+INT":
					uniqueAttribs.append(indexAttrib.name + "_ms")
		return uniqueAttribs



class Class(TypeBase):
	def __init__(self, name, base = None):
		TypeBase.__init__(self, name)
		self.parents = []
		self.rawname = ""
		self.base = base
		self.documentation = ""
		self._index = None
		self._isRoot = False
		self._isPublicObject = False
		self._ownTable = False

	def setRootElement(self, root):
		self._isRoot = root

	def setPublic(self):
		self._isPublicObject = True

	def setOwnTable(self, table):
		self._ownTable = table

	def hasOwnTable(self):
		return self._ownTable

	def isRootElement(self):
		return self._isRoot

	def hasParent(self):
		return len(self.parents) > 0

	def hasParents(self):
		return len(self.parents) > 1

	def getParentName(self):
		if self.hasParents():
			return self.parent[0]
		else:
			return ""

	def getParents(self):
		return self.parents;

	def getRootElement(self):
		if self.isSimpleType():
			return None
		
		if self.hasParents():
			return None

		element = self
		while element.hasParent():
			element = element.parents[0]
			if element.hasParents():
				return None

		return element

	def getPathToRoot(self):
		path = []
		element = self
		path.append(element)
		while not element.parent is None:
			element = element.parent
			path.append(element)

		return path

	def isSimpleType(self):
		return not self.hasParent() and not self.hasElements()

	def isPublicType(self):
		#return self.hasElements()
		return self._isPublicObject

	def addIndex(self, name, unique):
		if not name:
			return False

		if self._index:
			return False

		self._index = Index(name)
		self._index.unique = unique

		print "INFO: added ",
		if unique:
			print "unique ",
		print "index to '%s'" % self.name
		
		return True

	def hasIndex(self):
		return (not self._index is None) and (not self._index.isEmpty())

	def addIndexAttribute(self, name):
		if self._attributes.has_key(name):
			attrib = self._attributes[name]
			self._index.addAttribute(attrib)
		else:
			print("WARNING: index attribute '%s' in '%s' not found => ignoring" % (name, self.name))

	def isIndexAttribute(self, attrib):
		if self._index is None:
			return False;
		return self._index.hasAttribute(attrib)

	def getIndexAttributeCount(self):
		if self._index is None:
			return 0
		return len(self._index.getAttributes())

	def getIndexAttributes(self):
		if self._index is None:
			return []
		return self._index.getAttributes()


	def getIndexNames(self, start=None, end=None):
		if self._index is None:
			return ""
		return ", ".join( [attribute.Name() for attribute in self._index.getAttributes()[start:end]] )

	def getIndexNames2(self, format=False, start=None, end=None):
		if self._index is None:
			return ""
		if not format:
			return "".join( ["[%s]" % attribute.Name() for attribute in self._index.getAttributes()[start:end]] )
		else:
			return "".join( ["[%s]" for attribute in self._index.getAttributes()[start:end]] )


	def isCompleteIndex(self, attribs):
		if self._index is None:
			return False

		tmpIndexAttribs = set(self._index.getAttributes())
		#tmpIndexAttribs.sort()

		#tmpAttribs = list(attribs)
		#tmpAttribs.sort()
		tmpAttribs = set(attribs)

		return tmpIndexAttribs.issubset(tmpAttribs)


class Scheme(ElementContainer):
	def __init__(self, name):
		ElementContainer.__init__(self)
		self.name = name
		self._types = dict()
		self._typeList = []
		self._enums = dict()
		self._enumList = []

	def Name(self):
		return self.name

	def addElement(self, element):
		self._elements.append(element)

	def addType(self, type):
		if self._types.has_key(type.name):
			print "ERROR: type '%s' already defined in scheme '%s'" % (type.name, self.name)
			return
			
		self._types[type.name] = type

		index = 0
		for element in self._typeList:
			if element.dependsOn(type):
				self._typeList.insert(index, type)
				return

			index = index+1

		self._typeList.append(type)

	def addEnum(self, enum):
		if self._enums.has_key(enum.name):
			print "ERROR: enum '%s' already defined in scheme '%s'" % (enum.name, self.name)
			return

		self._enums[enum.name] = enum
		self._enumList.append(enum)

	def hasTypes(self):
		return len(self._typeList) > 0

	def getTypes(self):
		return self._typeList
	
	def getType(self, name):
		if self._types.has_key(name):
			return self._types[name]
		
		return None

	def hasEnums(self):
		return len(self._enumList) > 0
	
	def getEnums(self):
		return self._enumList
	
	def getEnum(self, name):
		if self._enums.has_key(name):
			return self._enums[name]
		
		return None

	def getElementsFlat(self):
		ec = ElementContainer.getElementsFlat(self)
		#ec += self._typeList

		return ec

	def getElementOrType(self, name):
		element = ElementContainer.getElement(self, name)
		if element is None:
			for type in self._typeList:
				if type.name == name:
					return type
		return element


class Config:
	def __init__(self):
		self.outputDirectory = 'datamodel'
		self.interfaceOutputDirectory = 'swig'
		self.outputRelative = True
		self.createPackageHeader = True
		self.createSWIGInterfaces = True
		
		self.namespace = 'Seiscomp'
		self.subNamespaces = ['DataModel']
		self.baseClass = 'Core::BaseObject'
		self.baseClassInclude = '<seiscomp3/core/baseobject.h>'
		self.enumInclude = '<seiscomp3/core/enumeration.h>'
		self.baseObject = 'Object'
		self.basePublicObject = 'PublicObject'
		self.publicIdType = 'string'
		self.publicIdAttrib = 'publicID'
		self.reflectionClass = 'Reflection'
		self.observerClass = 'Observer'
		self.messageInclude = '<seiscomp3/client/message.h>'
		self.include = 'seiscomp3/datamodel/%s.h'
		self.typeInclude = 'types'
		self.typeInclude = 'types'

		self.memberPrefix = '_'
		self.memberPostfix = ''
		
		self.pointerPrefix = ''
		self.pointerPostfix = 'Ptr'
		self.pointerAccess = '.get()'
		self.arrayInclude = 'vector'
		self.arrayPrefix = 'std::vector<'
		self.arrayPostfix = '>'
		self.optionalPrefix = 'OPT('
		self.optionalPostfix = ')'
		self.optionalNull = "NULL"
		self.optionalNone = "Seiscomp::Core::None"

		self.typeMap = dict()

		self.typeMap['string'] = TypeMapper('std::string', None, 'string', False)
		self.typeMap['isotime'] = TypeMapper('Seiscomp::Core::Time', None, 'seiscomp3/core/datetime.h')
		self.typeMap['datetime'] = TypeMapper('Seiscomp::Core::Time', None, 'seiscomp3/core/datetime.h')
		self.typeMap['boolean'] = TypeMapper('bool', 'false')
		self.typeMap['int'] = TypeMapper('int', '0')
		self.typeMap['float'] = TypeMapper('double', '0')
		self.typeMap['complex'] = TypeMapper('std::complex<double>', None, 'complex')



	def getSubnamespaceString(self, sep, upper):
		namespace = sep.join(self.subNamespaces)

		if upper:
			return namespace.upper()
		
		return namespace


	def getNamespaceString(self, sep, upper):
		namespace = self.getSubnamespaceString(sep, upper)
		if self.namespace:
			namespace = self.namespace + sep + namespace

		if upper:
			return namespace.upper()
		
		return namespace


	def getInclude(self, type):
		if self.typeMap.has_key(type):
			return self.typeMap[type].include

	def getIncludes(self, element):
		includes = set()
		attribs = element.getAttributes()
		for attribute in attribs:
			if self.typeMap.has_key(attribute.type):
				#if not self.typeMap[attribute.type].ownType:
				if self.typeMap[attribute.type].include:
					includes.add(self.typeMap[attribute.type].include)
				if attribute.isArray():
					includes.add(self.arrayInclude)

		if element.hasElements():
			includes.add(self.arrayInclude)

		return includes
	

	def splitString(self, line, width):
		current = 0
		lines = []

		#line = line.replace('\t', '').replace('\n',' ').replace('  ', ' ').strip()
		line = line.replace('\t', '').strip()

		#lines = line.split('\n').strip()
		#return lines

		while current < len(line):
			end = line.find('\n', current)
			if (end == -1) or (end - current > width):
				if len(line) - current > width:
					end = line.rfind(' ', current, current+width)
					if end == -1:
						end = line.find(' ', current)
						if end == -1:
							end = len(line)
				else:
					end = len(line)
	
			lines.append(line[current:end].strip())

			current = end + 1

		return lines

	def translateAccessor(self, element, attr):
		return self.translatePublicMember(element.getAttribute(attr).Name()) + "()"

	def translateMember(self, m):
		return self.memberPrefix + decapitalizeFirst(m) + self.memberPostfix

	def translatePublicMember(self, m):
		return decapitalizeFirst(m)

	def translateArrayMember(self, m):
		return self.memberPrefix + decapitalizeFirst(m) + "s" + self.memberPostfix

	def translatePublicArrayMember(self, m):
		return decapitalizeFirst(m) + "s"

	def translateType(self, t):
		if self.typeMap.has_key(t):
			#if self.typeMap[t].ownType:
			#	return self.translatePtr(self.typeMap[t].name)
			#else:
			return self.typeMap[t].name
		
		return t

	def translateOptionalType(self, t):
		#if isinstance(t, TypeBase):
		#	return t.Name() + "*"
		if isinstance(t, TypeBase):
			return self.optionalPrefix + t.Name() + self.optionalPostfix

		if self.typeMap.has_key(t):
			#if self.isComplexType(t):
			#	return self.typeMap[t].name + "*"
			#else:
			#	return self.optionalPrefix + self.typeMap[t].name + self.optionalPostfix
			return self.optionalPrefix + self.typeMap[t].name + self.optionalPostfix
		
		return t

	def isComplexType(self, t):
		if self.typeMap.has_key(t):
			return self.typeMap[t].ownType and not self.typeMap[t].primitive
		return False

	def translateOptionalMemberType(self, t):
		if self.typeMap.has_key(t):
			#if self.isComplexType(t):
			#	return self.translatePtr(self.typeMap[t].name)
			#else:
			#	return self.optionalPrefix + self.typeMap[t].name + self.optionalPostfix
			return self.optionalPrefix + self.typeMap[t].name + self.optionalPostfix
		
		print "WARNING: unknown type (%s)" % t
		return t
	
	def translateOptionalNullValue(self, t):
		if isinstance(t, TypeBase):
			return self.optionalNull

		if self.typeMap.has_key(t):
			if self.isComplexType(t):
				return self.optionalNull
			else:
				return self.optionalNone
		
		return self.optionalNull

	def translateArrayType(self, t):
		return self.arrayPrefix + " " + self.translateType(t) + " " + self.arrayPostfix

	def getAttributeType(self, attribute):
		if attribute.isOptional():
			return self.translateOptionalType(attribute.type)
		else:
			return self.translateType(attribute.type)

	def getAttributeMemberType(self, attribute):
		if attribute.isArray():
			return self.translateArrayType(attribute.type)

		if attribute.isOptional():
			return self.translateOptionalMemberType(attribute.type)
		else:
			return self.translateType(attribute.type)

	def getTypes(self):
		return self.typeMap.keys()

	def getType(self, name):
		if self.typeMap.has_key(name):
			return self.typeMap[name]

	def isRefType(self, t):
		if self.typeMap.has_key(t):
			return self.typeMap[t].ownType and not self.typeMap[t].primitive

		return False

	def getTypeDefault(self, t):
		if self.typeMap.has_key(t):
			return self.typeMap[t].default
		
		return None

	def translatePtr(self, name):
		return self.pointerPrefix + name + self.pointerPostfix

	def translateParamType(self, t):
		if self.typeMap.has_key(t):
			t_ = self.typeMap[t]
			if t_.primitive:
				return t_.name
			else:
				#if t_.ownType:
				#	return t_.name + "*"
				#else:
				#	return "const " + t_.name + "&"
				return "const " + t_.name + "&"
		
		return t


	def translateConstParamType(self, t):
		if self.typeMap.has_key(t):
			t_ = self.typeMap[t]
			if t_.primitive:
				return t_.name
			else:
				#if t_.ownType:
				#	return t_.name + "*"
				#else:
				#	return "const " + t_.name + "&"
				return "const " + t_.name + "&"
		
		return t


	def translateReturnAttrib(self, attrib, isConst):
		if self.typeMap.has_key(attrib.type):
			t_ = self.typeMap[attrib.type]
			if t_.primitive and not attrib.isOptional():
				if isConst:
					if t_.type and t_.type.isSimpleType():
						return "const " + t_.name + "&"
					return t_.name
				else:
					return t_.name + "&"
			else:
				if t_.ownType and not t_.primitive and attrib.isOptional():
					if isConst:
						#return "const " + t_.name + "*"
						return "const " + t_.name + "&"
					else:
						#return t_.name + "*"
						return t_.name + "&"
				else:
					if isConst:
						if t_.type and t_.type.isSimpleType():
							return "const " + t_.name + "&"
						else:
							if not t_.type and not t_.primitive:
								return "const " + t_.name + "&"
						return self.translateType(attrib.type)
					else:
						return self.translateType(attrib.type) + "&"
		
		return attrib.type

	def translateConstParamAttribute(self, attrib):
		if self.typeMap.has_key(attrib.type):
			t_ = self.typeMap[attrib.type]
			if t_.primitive and not attrib.isOptional():
				return t_.name
			else:
				#if t_.ownType and not t_.primitive and attrib.isOptional():
				#	return t_.name + "*"
				#else:
				#	return "const " + self.getAttributeType(attrib) + "&"
				return "const " + self.getAttributeType(attrib) + "&"
		
		return attrib.type


	def translateParamAttribute(self, attrib):
		if self.typeMap.has_key(attrib.type):
			t_ = self.typeMap[attrib.type]
			if t_.primitive and not attrib.isOptional():
				return t_.name
			else:
				#if t_.ownType and not t_.primitive and attrib.isOptional():
				#	return t_.name + "*"
				#else:
				#	return self.getAttributeType(attrib) + "&"
				return self.getAttributeType(attrib) + "&"
		
		return attrib.type


	def makeParam(self, attribute, type = True):
		if type:
			return self.translateConstParamAttribute(attribute) + " " + attribute.name
		else:
			return attribute.name


	def makeParamList(self, attributes, types = True):
		list = ""
		seperator = ""
		for attrib in attributes:
			list = list + seperator + self.makeParam(attrib)
			seperator = ", "

		return list


	def convertToSpaces(self, s):
		s_ = ""
		for i in range(0,len(s)):
			s_ += " "
		return s_

	def convertToUnderline(self, s):
		return s
		#return re.sub(r"(?<=[a-z])([A-Z])", r"_\1", s).lower()

	def decapitalizeFirst(self, s):
		return decapitalizeFirst(s)
