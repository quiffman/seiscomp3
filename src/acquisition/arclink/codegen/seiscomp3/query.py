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

from data import *

class Query:
	def __init__(self, name):
		self.name = name
		self.parameters = dict()
		self.parametersList = []
		self.sources = set()
		self.joins = []
		self.conditions = []
		self.result = None
		self.groupBy = None
		self.typeJoins = dict()
		self._rootElement = None
		self.documentation = ""

		words = self.name.split('_')
		cWords = []
		
		for word in words:
			cWords.append(capitalizeFirst(word))

		self._queryName = "".join(cWords)

	def Name(self):
		return self._queryName

	def rootElement(self):
		return self._rootElement

	def addParam(self, param):
		if param is None:
			return

		if param.target is None:
			return

		if param.target.boundToElement():
			if not self.addSource(param.target.type):
				return False

		self.parameters[param.name] = param
		self.parametersList.append(param)

		return True

	def getParam(self, name):
		if self.parameters.has_key(name):
			return self.parameters[name]
		return None

	def params(self):
		return self.parametersList

	# Returns all parameters that are bound to
	# an element attribute
	def boundParams(self):
		boundList = []
		for param in self.parametersList:
			if param.target.boundToAttribute():
				boundList.append(param)
		return boundList

	# Returns all elements that are bound by a parameter
	def boundParamElements(self):
		boundSet = set()
		for param in self.parametersList:
			if param.target.boundToAttribute():
				boundSet.add(param.target.type)
		return boundSet

	# Returns all elements that have to be
	# used in the parameter list
	def paramElements(self):
		typeSet = set()
		for param in self.parametersList:
			if param.target.boundToAttribute():
				typeSet.add(param.target.type)

		sortedTypes = []

		lastType = None
		while len(typeSet):
			for type in typeSet:
				# Found a parameter type as starting point
				if type.parent == lastType or not type.parent in typeSet:
					lastType = type
					typeSet.remove(type)
					break

			if lastType is None:
				break

			sortedTypes.append(lastType)

		return sortedTypes

	def setParam(self, name, value):
		param = self.getParam(name)
		if param:
			param.value = value

	def addSource(self, source):
		if source != None:
			self.sources.add(source)
			if not self._rootElement:
				self._rootElement = source.getRootElement()
			elif self._rootElement != source.getRootElement():
				print "ERROR: elements from different packages cannot be mixed in one query"
				return False

		return True

	def hasSource(self, type):
		return type in self.sources

	def getSources(self):
		return self.sources

	def sourceNames(self, config):
		srcNames = []
		for source in self.sources:
			srcNames.append(source.name)
			if source.isPublicType() and (self.hasConditionForElementAttribute(source, config.publicIdAttrib) or self.result.type == source):
				srcNames.append(config.basePublicObject + " as " + config.basePublicObject[0] + source.name)
		return srcNames

	def addJoin(self, join):
		if join != None:
			self.joins.append(join)
			for op in join.operands:
				if op.boundToElement():
					if not self.addSource(op.type):
						return False

				if op.boundToAttribute():
					if not self.typeJoins.has_key(op.type):
						self.typeJoins[op.type] = []
					self.typeJoins[op.type].append(op.type.getAttribute(op.attribute))

		return True

	def typeAttributesInJoin(self, type):
		if not self.typeJoins.has_key(type):
			return []
		return self.typeJoins[type]

	def operandElementsInJoin(self, type):
		elements = set()
		for join in self.joins:
			foundOp = False
			for op in join.operands:
				if op.type == type:
					foundOp = True
					break
			if foundOp:
				for op in join.operands:
					if op.type != type:
						elements.add(op.type)

		return elements

	def operandAttributesInJoin(self, type, operand):
		attributes = set()
		for join in self.joins:
			foundOp = False
			for op in join.operands:
				if op.type == type:
					foundOp = True
					break
			if foundOp:
				for op in join.operands:
					if op.type == operand:
						attr = op.type.getAttribute(op.attribute)
						if attr:
							attributes.add(attr)

		return attributes

	# Returns the attribute in the join with operand.attribute for
	# element
	# 'element', 'operand' and 'attribute' has to be class instances
	def joinAttribute(self, element, operand, attribute):
		for join in self.joins:
			for op in join.operands:
				if op.type == operand and op.attribute == attribute.name:
					for op2 in join.operands:
						if op2.type == element:
							return op2.attribute
		return None

	def hasJoins(self):
		return len(self.joins) > 0

	def addCondition(self, cond):
		if cond != None:
			self.conditions.append(cond)

	def hasConditions(self):
		return len(self.conditions) > 0

	def hasConditionFor(self, param):
		if isinstance(param, Parameter):
			for cond in self.conditions:
				if isinstance(cond, Condition):
					if cond.paramName == param.name and cond.attribute:
						if cond.attribute.type == param.target.type and cond.attribute and cond.attribute.attribute == param.target.attribute:
							return True
				elif cond.hasConditionFor(param):
						return True
		else:
			for cond in self.conditions:
				if isinstance(cond, Condition):
					if cond.param == param:
						return True
				elif cond.hasConditionFor(param):
						return True

		return False

	def conditionsFor(self, element):
		conditions = []
		for cond in self.conditions:
			if isinstance(cond, Condition):
				if cond.attribute and cond.attribute.type == element:
					conditions.append(cond)
			else:
				conditions = conditions + cond.conditionsFor(element)

		return conditions

	def hasConditionForElementAttribute(self, element, attributename):
		for cond in self.conditions:
			if isinstance(cond, Condition):
				if cond.attribute:
					if cond.attribute.attribute == attributename and cond.attribute.type == element:
						return True
			elif cond.hasConditionForElementAttribute(element, attributename):
				return True

		for join in self.joins:
			for op in join.operands:
				if op.type == element and op.attribute == attributename:
					return True

		return False

	def setResult(self, attribute, groupAttribute=None):
		self.result = attribute
		self.groupBy = groupAttribute

	def hasResult(self):
		return self.result != None

	def isValueQuery(self):
		return self.result.boundToAttribute() or self.result.boundToAllAttributes()


class ElementAttribute:
	def __init__(self, type, attribute):
		self.type = type
		self.attribute = attribute

	def boundToAttribute(self):
		return self.boundToElement() and (self.attribute != None and self.attribute != "*")

	def boundToAllAttributes(self):
		return self.boundToElement() and (self.attribute != None and self.attribute == "*")

	def boundToIndexAttribute(self):
		if self.boundToAttribute():
			if self.type.isIndexAttribute(self.type.getAttribute(self.attribute)):
				return True

		return False

	def boundToElement(self):
		return self.type != None and isinstance(self.type, TypeBase)

	def getType(self):
		if self.boundToAttribute():
			return self.type.getAttribute(self.attribute).type
		return ""


class Parameter:
	def __init__(self, name, elementAttribute):
		self.name = name
		self.target = elementAttribute
		self.value = None


class Join:
	def __init__(self):
		self.operands = []

	def addOperand(self, op):
		if op != None:
			self.operands.append(op)


class ConditionGroup:
	def __init__(self, operation):
		self.operation = operation
		self.conditions = []

	def addCondition(self, cond):
		if cond != None:
			self.conditions.append(cond)

	def hasConditionFor(self, param):
		if isinstance(param, Parameter):
			for cond in self.conditions:
				if isinstance(cond, Condition):
					if cond.paramName == param.name and cond.attribute:
						if cond.attribute.type == param.target.type and cond.attribute.attribute == param.target.attribute:
							return True
				elif cond.hasConditionFor(param):
						return True
		else:
			for cond in self.conditions:
				if isinstance(cond, Condition):
					if cond.param == param:
						return True
				elif cond.hasConditionFor(param):
						return True

		return False

	def hasConditionForElement(self, element):
		for cond in self.conditions:
			if isinstance(cond, Condition):
				if cond.attribute and cond.attribute.type == element:
					return True
			elif cond.hasConditionForElement(element):
					return True

		return False

	def hasConditionForElementAttribute(self, element, attributename):
		for cond in self.conditions:
			if isinstance(cond, Condition):
				if cond.attribute:
					if cond.attribute.attribute == attributename and cond.attribute.type == element:
						return True
			elif cond.hasConditionForElementAttribute(element, attributename):
				return True

		return False

	def conditionsFor(self, element):
		conditions = []
		if not self.hasConditionForElement(element):
			return conditions

		conditions.append(ConditionGroup(self.operation))
		for cond in self.conditions:
			if isinstance(cond, Condition):
				if cond.attribute.type == element:
					conditions.append(cond)
			else:
				conditions = conditions + cond.conditionsFor(element)

		conditions.append(None)
		return conditions

class Condition:
	def __init__(self, paramName, operation, elementAttribute):
		self.paramName = paramName
		self.operation = operation
		self.attribute = elementAttribute


class QuerySQLConverter:
	def __init__(self, query):
		self._query = query
		self._arguments = []

	def arguments(self):
		return self._arguments;

	def toString(self, scheme, config):
		self._arguments = []

		q = "select "
		if self._query.result.type.isPublicType():
			q += config.basePublicObject[0] + self._query.result.type.name + "." + config.publicIdAttrib + ","
		q += self._query.result.type.name + "."
		if self._query.result.attribute:
			q += self._query.result.attribute
			attribute = self._query.result.type.getAttribute(self._query.result.attribute)
			if attribute:
				schemeType = scheme.getType(attribute.type)
				if schemeType:
					if len(schemeType.getAttributes()) > 1:
						primary = schemeType.getPrimaryAttribute()
						if primary:
							q += "_" + primary.name
					else:
						q += "_" + schemeType.getAttributes()[0].name
		else:
			q += "*"

		q += " from "
		
		q += ",".join(self._query.sourceNames(config))

		whereClauseStarted = False

		if self._query.hasJoins():
			joins = []
			for join in self._query.joins:
				s = ""
				last_str = ""
				for op in join.operands:
					attr = op.attribute
					if attr == config.publicIdAttrib:
						current_str = config.basePublicObject[0] + op.type.name + "." + attr
					else:
						current_str = op.type.name + "." + attr

					if last_str:
						joins.append(last_str + "=" + current_str)

					last_str = current_str

			if len(joins) > 0:
				q += " where "
				q += " and ".join(joins)
				whereClauseStarted = True

		for source in self._query.getSources():
			joins = []
			if source.isPublicType() and (self._query.hasConditionForElementAttribute(source, config.publicIdAttrib) or self._query.result.type == source):
				joins.append(source.name + "._oid=" + config.basePublicObject[0] + source.name + "._oid")
			if len(joins) > 0:
				if whereClauseStarted:
					q += " and "
				else:
					q += " where "
				q += " and ".join(joins)
				whereClauseStarted = True
		
		if self._query.hasConditions():
			conds = []
			for cond in self._query.conditions:
				if isinstance(cond, Condition):
					conds.append(self.conditionToString(scheme, config, cond))
				elif isinstance(cond, ConditionGroup):
					conds.append(self.conditionGroupToString(scheme, config, cond))

			if whereClauseStarted:
				q += " and "
			else:
				q += " where "

			q += " and ".join(conds)

		if self._query.groupBy:
			q += " group by "
			if self._query.result.type.isPublicType():
				q += config.basePublicObject[0]
			q += self._query.result.type.name + "." + self._query.groupBy

		return q

	def toSource(self, config):
		operationMapper = dict()
		operationMapper['eq'] = " == "
		operationMapper['like'] = " == "
		operationMapper['lt'] = " < "
		operationMapper['le'] = " <= "
		operationMapper['ge'] = " >= "
		operationMapper['gt'] = " > "
		operationMapper['is_unset'] = "!"

		logOperationMapper = dict()
		logOperationMapper['and'] = " && "
		logOperationMapper['or'] = " || "
		
		ident = ""
		lookUpSources = []
		lookUpSources.append(self._query.rootElement())
		paramElements = self._query.paramElements()
		indexVar = 0;
		bracketStack = []
		identValue = "    "

		retType = "int"
		if self._query.result.attribute:
			attrib = self._query.result.type.getAttribute(self._query.result.attribute)
			retType = config.translateOptionalType(attrib.type)

		arguments = []
		arguments.append(self._query.rootElement().Name() + " *" + self._query.rootElement().name)
		for param in self._query.params():
			if param.target.boundToAttribute():
				arguments.append(config.translateConstParamType(param.target.type.getAttribute(param.target.attribute).type) + " " + param.name)
			else:
				arguments.append(config.translateConstParamType(param.target.type) + " " + param.name)

		print ident + str(retType) + " get" + self._query.Name() + "(" + ", ".join(arguments) + ") {"
		ident += identValue

		indexLookup = True
		
		for paramElement in paramElements:
			conditions = self._query.conditionsFor(paramElement)
			andconds = []
			ignore = False
			for cond in conditions:
				if isinstance(cond, Condition) and not ignore:
					if cond.operation == "eq" or cond.operation == "like":
						andconds.append(cond)
				elif cond:
					ignore = True
				else:
					ignore = False

			if paramElement.isCompleteIndex([paramElement.getAttribute(i.attribute.attribute) for i in andconds]):
				paramNameList = []
				for attr in paramElement.getIndexAttributes():
					for cond in andconds:
						if cond.attribute.type == paramElement and cond.attribute.attribute == attr.name:
							paramNameList.append(cond.paramName)
				
				paramList = ",".join(paramNameList)
				print ident + "%s* %s = %s->%s(%s(%s));" % (paramElement.Name(), paramElement.name, paramElement.parent.name, config.translatePublicMember(paramElement.Name()), paramElement.Name() + "Index", paramList)
				if not indexLookup:
					print ident + "if ( %s == NULL ) continue;" % paramElement.name
				else:
					print ident + "if ( %s == NULL ) return %s;" % (paramElement.name, config.translateOptionalNullValue(self._query.result.getType()))
				bracketStack.append(None)
				for andcond in andconds:
					conditions.remove(andcond)
			else:
				print ident + "for ( size_t i%d = 0; i%d < %s->%sCount(); ++i%d ) {" % (indexVar, indexVar, paramElement.parent.name, config.translatePublicMember(paramElement.Name()), indexVar)
				bracketStack.append(ident)
				ident = ident + identValue
				print ident + "%s* %s = %s->%s(i%d);" % (paramElement.Name(), paramElement.name, paramElement.parent.name, config.translatePublicMember(paramElement.Name()), indexVar)
				indexVar += 1
				indexLookup = False
			
			lookUpSources.append(paramElement)

			condSem = 0
			condStr = ""
			opStack = []
			currOp = ""
			currOpStr = ""

			for cond in conditions:
				if isinstance(cond, Condition):
					if cond.paramName:
						condStr += "%s%s%s%s" % (currOpStr, cond.paramName, operationMapper[cond.operation], "%s->%s" % (paramElement.name, config.translateAccessor(paramElement, cond.attribute.attribute)))
					else:
						condStr += "%s%s%s" % (currOpStr, operationMapper[cond.operation], "%s->%s" % (paramElement.name, config.translateAccessor(paramElement, cond.attribute.attribute)))
					currOpStr = currOp
				elif cond:
					condStr += currOpStr + "("
					condSem = condSem + 1
					opStack.append(currOp)
					currOp = logOperationMapper[cond.operation]
					currOpStr = ""
				else:
					condStr += ")"
					condSem = condSem - 1
					currOp = opStack.pop()
					currOpStr = ""

				if condSem == 0:
					if not indexLookup:
						print ident + "if ( !(%s) ) continue;" % condStr
					else:
						print ident + "if ( !(%s) ) return %s;" % (condStr, config.translateOptionalNullValue(self._query.result.getType()))
					condStr = ""

			for param in self._query.boundParams():
				if param.target.type == paramElement:
					joinElements = self._query.operandElementsInJoin(paramElement)

					while len(joinElements):
						removedElement = False
						for element in joinElements:
							if element.parent in lookUpSources:
								lookUpSources.append(element)
								joinElements.remove(element)
								removedElement = True

								joinAttributes = self._query.operandAttributesInJoin(element, paramElement)
								joinOpAttributes = self._query.operandAttributesInJoin(paramElement, element)
								fullIndex = ""
								if element.isCompleteIndex(joinOpAttributes):
									fullIndex = "*index lookup*"
									print ident + "%s* %s = %s->%s(%s(%s));" % (element.Name(), element.name, element.parent.name, config.translatePublicMember(element.Name()), element.Name() + "Index", ", ".join([paramElement.name + "->" + config.translateAccessor(paramElement, self._query.joinAttribute(paramElement, element, i)) for i in element.getIndexAttributes()]))
									print ident + "if ( %s == NULL ) continue;" % element.name
								elif len(joinOpAttributes) == 0:
									break
								else:
									print ident + "*this join (%s) is not yet implemented" % ",".join([i.name for i in joinOpAttributes])

								if element == self._query.result.type:
									print ident + "return %s->%s;" % (element.name, config.translateAccessor(element, self._query.result.attribute))

								break

						if not removedElement:
							print "ERROR: could not resolve joins"
							break

			if paramElement == self._query.result.type:
				print ident + "return %s->%s;" % (paramElement.name, config.translateAccessor(paramElement, self._query.result.attribute))

		while len(bracketStack):
			e = bracketStack.pop()
			if e:
				ident = e
				print ident + "}"

		print ident + "return %s;" % config.translateOptionalNullValue(self._query.result.getType())

		ident = ""
		print ident + "}"

	def conditionToString(self, scheme, config, cond):
		operationMapper = dict()
		operationMapper['eq'] = "="
		operationMapper['like'] = "like"
		operationMapper['lt'] = ">"
		operationMapper['le'] = ">="
		operationMapper['ge'] = "<="
		operationMapper['gt'] = "<"
		operationMapper['ne'] = "!="

		if cond.operation == 'is_unset':
			return cond.attribute.type.name + "." + cond.attribute.attribute + " is null"
		elif cond.operation == 'is_set':
			return cond.attribute.type.name + "." + cond.attribute.attribute + " is not null"

		if not operationMapper.has_key(cond.operation.lower()):
			return "1"

		param = self._query.getParam(cond.paramName)
		value = "'%s'"
		self._arguments.append(cond.paramName)
		#if param and param.value:
		#	value = "'" + str(param.value) + "'"

		elementAttribute = None
		if cond.attribute and cond.attribute.boundToAttribute():
			elementAttribute = cond.attribute
		elif param:
			elementAttribute = param.target

		attr = elementAttribute.attribute

		attribute = elementAttribute.type.getAttribute(attr)
		if attribute:
			schemeType = scheme.getType(attribute.type)
			if schemeType:
				if len(schemeType.getAttributes()) > 1:
					primary = schemeType.getPrimaryAttribute()
					if primary:
						attr = attr + "_" + primary.name
				else:
					attr = attr + "_" + schemeType.getAttributes()[0].name

		if attr == config.publicIdAttrib:
			table = config.basePublicObject[0] + elementAttribute.type.name
		else:
			table = elementAttribute.type.name

		return table + "." + attr + operationMapper[cond.operation.lower()] + value

	def conditionGroupToString(self, scheme, config, group):
		conds = []
		for cond in group.conditions:
			if isinstance(cond, Condition):
				conds.append(self.conditionToString(scheme, config, cond))
			elif isinstance(cond, ConditionGroup):
				conds.append(self.conditionGroupToString(scheme, config, cond))
		
		if group.operation.lower() == "and":
			return "(" + " and ".join(conds) + ")"
		elif group.operation.lower() == "or":
			return "(" + " or ".join(conds) + ")"
		return ""
