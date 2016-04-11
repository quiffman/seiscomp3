#*****************************************************************************
# dbupdate.py
#
# Update Seiscomp 3 DB using keyfiles
#
# (c) 2007 Andres Heinloo, GFZ Potsdam
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any later
# version. For more information, see http://www.gnu.org/
#*****************************************************************************

import os
import re
import sys
import time
import glob
import fnmatch
import decimal
import optparse
from seiscomp import logs
from seiscomp.keyfile import Keyfile
from seiscomp.nettab import Instruments, NettabError
from seiscomp.db.seiscomp3 import sc3wrap
from seiscomp.db.seiscomp3.inventory import Inventory
from seiscomp3 import Core, Client, DataModel, Logging

# For compatibility with python 2.3
from sets import Set as set

def sortDictionary(dict):
    newDict = {}
    sortedKeys = dict.keys()
    sortedKeys.sort()

    for key in sortedKeys:
        newDict[key] = dict[key]
    return newDict
# end 


gainTable = {}

def loadGains(fileName):
    try:
        fd = open(fileName)
        try:
            line = fd.readline()
            lineno = 0
            while line:
                line = line.strip()
                lineno += 1

                if not line or line[0] == '#':
                    line = fd.readline()
                    continue
                
                try:
                    (deviceName, deviceIdPattern, streamPattern, gain) = line.split()
                    if deviceName in gainTable:
                        gainTable[deviceName].append((streamPattern, deviceIdPattern,
                            float(gain)))
                    else:
                        gainTable[deviceName] = [ (streamPattern, deviceIdPattern,
                            float(gain)) ]
                
                except (TypeError, ValueError):
                    logs.error("%s:%d: parse error" % (fileName, lineno))
                    sys.exit(1)

                line = fd.readline()
        
        finally:
            fd.close()
    
    except IOError, e:
        logs.error("cannot open %s: %s" % (fileName, str(e)))
        sys.exit(1)

def getGain(datalogger, dataloggerId, seismometer, seismometerId, streamCode):
    try:
        if datalogger == "DUMMY":
            dataloggerGain = 1.0
        
        else:
            for (streamPattern, dataloggerIdPattern, dataloggerGain) in gainTable[datalogger]:
                if fnmatch.fnmatch(streamCode, streamPattern) and \
                    fnmatch.fnmatch(dataloggerId, dataloggerIdPattern):
                    break
            
            else:
                logs.error("cannot find gain for %s, %s, %s" % (datalogger,
                    dataloggerId, streamCode))

                return 0
        
        if seismometer == "DUMMY":
            seismometerGain = 1.0

        else:
            for (streamPattern, seismometerIdPattern, seismometerGain) in gainTable[seismometer]:
                if fnmatch.fnmatch(streamCode, streamPattern) and \
                    fnmatch.fnmatch(seismometerId, seismometerIdPattern):
                    break
            
            else:
                logs.error("cannot find gain for %s, %s, %s" % (seismometer,
                    seismometerId, streamCode))

                return 0

    except KeyError, e:
        logs.error("cannot find gain for " + str(e))
        return 0

    return dataloggerGain * seismometerGain
        
_rx_samp = re.compile(r'(?P<bandCode>[A-Z])?(?P<sampleRate>.*)$')

def _normalize(num, denom):
    if num > denom:
        (a, b) = (num, denom)
    else:
        (a, b) = (denom, num)

    while b > 1:
        (a, b) = (b, a % b)

    if b == 0:
        return (num / a, denom / a)

    return (num, denom)

def _rational(x):
    sign, mantissa, exponent = x.as_tuple()
    sign = (1, -1)[sign]
    mantissa = sign * reduce(lambda a, b: 10 * a + b, mantissa)
    if exponent < 0:
        return _normalize(mantissa, 10 ** (-exponent))
    else:
        return (mantissa * 10 ** exponent, 1)

def parseSampling(sampling):
    compressionLevel = "2"
    instrumentCode = "H"
    locationCode = ""
    endPreamble = sampling.find('_')
    if endPreamble > 0:
        for x in sampling[:endPreamble].split('/'):
            if x[0] == 'F':
                compressionLevel = x[1:]
            elif x[0] == 'L':
                locationCode = x[1:]
            elif x[0] == 'T':
                instrumentCode = x[1:]
            else:
                logs.warning("unknown code %s in %s" % (x[0], sampling))
        
    if not sampling[endPreamble+1:]:
        return

    for x in sampling[endPreamble+1:].split('/'):
        m = _rx_samp.match(x)
        if not m:
            logs.error("error parsing sampling %s at %s" % (sampling, x))
            continue
            
        try:
            sampleRate = decimal.Decimal(m.group('sampleRate'))
        except decimal.InvalidOperation:
            logs.error("error parsing sampling %s at %s" % (sampling, x))
            continue

        bandCode = m.group('bandCode')
        if not bandCode:
            if sampleRate >= 80:
                bandCode = 'H'
            elif sampleRate >= 40:
                bandCode = 'S'
            elif sampleRate > 1:
                bandCode = 'B'
            elif sampleRate == 1:
                bandCode = 'L'
            elif sampleRate == decimal.Decimal("0.1"):
                bandCode = 'V'
            elif sampleRate == decimal.Decimal("0.01"):
                bandCode = 'U'
            else:
                logs.error("could not determine band code for %s in %s" (x, sampling))
                continue
        
        yield (( bandCode + instrumentCode, locationCode ) +
            _rational(sampleRate) + ("Steim" + compressionLevel,))

def parseOrientation(orientation):
    for x in orientation.split(';'):
        try:
            (code, azimuth, dip) = x.split()
            yield (code, float(azimuth), float(dip))
        except (TypeError, ValueError):
            logs.error("error parsing orientation %s at %s" % (orientation, x))
            continue

_rx_route = re.compile(r'([^,(]+)(\(([^)]*)\))?,?')

def parseRoute(s):
    prio = 1
    pos = 0
    while pos < len(s):
        m = _rx_route.match(s, pos)
        if m == None:
            print >>sys.stderr, "error parsing route %s at %s" % (s, s[pos:])
            return

        if m.group(2):
            pat = tuple((m.group(3).split(',') + ['', '', '', ''])[:4])

        else:
            pat = ('*', '', '', '')

        yield (m.group(1), prio, pat)
        prio += 1
        pos = m.end()

_doy = (0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365)

def _is_leap(y):
    return (y % 400 == 0 or (y % 4 == 0 and y % 100 != 0))

def _ldoy(y, m):
    return _doy[m] + (_is_leap(y) and m >= 2)

def _dy2mdy(doy, year):
    month = 1
    while doy > _ldoy(year, month):
        month += 1

    mday = doy - _ldoy(year, month - 1)
    return (month, mday)

_rx_datetime = re.compile(r'(?P<year>[0-9]+)/(?P<doy>[0-9]+)(:(?P<hour>[0-9]{2})(?P<minute>[0-9]{2})(?P<second>[0-9]{2})?)?$')

def parseDate(datestr):
    m = _rx_datetime.match(datestr)
    if not m:
        logs.error("invalid date: " + datestr)
        return (Core.Time(1980, 1, 1, 0, 0, 0), "1980-01-01T00:00:00.0000Z")
    
    try:
        year = int(m.group("year"))
        (month, mday) = _dy2mdy(int(m.group("doy")), year)

        if m.group("hour"):
            hour = int(m.group("hour"))
            minute = int(m.group("minute"))
        else:
            hour = 0
            minute = 0

        if m.group("second"):
            second = int(m.group("second"))
        else:
            second = 0
            
        coretime = Core.Time(year, month, mday, hour, minute, second)
    
    except (TypeError, ValueError, IndexError, Core.ValueException):
        logs.error("invalid date: " + datestr)
        return (Core.Time(1980, 1, 1, 0, 0, 0), "1980-01-01T00:00:00.0000Z")
    
    return (coretime, coretime.toString("%Y-%m-%dT%H:%M:%S.%fZ"))
 
_rx_pkg = re.compile(r'(?P<pkg>[^:\s]+)(:(?P<profile>[^\s]+))?$')

def parsePkgstr(pkgstr):
    result = {}
    for x in pkgstr.split():
        m = _rx_pkg.match(x)
        if not m:
            logs.error("error parsing %s at %s" % (pkgstr, x))
            continue
        
        result[m.group('pkg')] = m.group('profile')
    
    if 'trunk' not in result:
        result['trunk'] = None
    
    return result

class ParsetWrapper(object):
    def __init__(self, parset):
        self.obj = parset
        self.parameters = {}

        for i in xrange(parset.parameterCount()):
            parameter = parset.parameter(i)
            self.parameters[parameter.publicID()] = parameter

    def update(self, pkgName, baseParsetName, kf):
        if pkgName:
            self.obj.setModuleID("Config/%s" % (pkgName,))
        else:
            self.obj.setModuleID("")

        if baseParsetName:
            self.obj.setBaseID("ParameterSet/" + baseParsetName)
        else:
            self.obj.setBaseID("")
        
        usedParameters = set()
        for (name, value) in kf.iteritems():
            publicID = "Parameter/" + self.obj.publicID().split('/', 1)[1] + "/" + name
            parameter = self.parameters.get(publicID)
            if parameter is None:
                parameter = DataModel.Parameter(publicID)
                self.obj.add(parameter)
                self.parameters[publicID] = parameter
            
            else:
                parameter.update()

            parameter.setName(name)
            parameter.setValue(value)
            usedParameters.add(publicID)

        for publicID in set(self.parameters.keys()) - usedParameters:
            self.obj.remove(self.parameters[publicID])
            del self.parameters[publicID]
        
class ConfigStationWrapper(object):
    def __init__(self, station):
        self.obj = station
    
    def update(self, parsetName):
        for i in xrange(self.obj.setupCount()):
            setup = self.obj.setup(i)
            if setup.name() == "default":
                setup.update()
                break
        
        else:
            setup = DataModel.Setup()
            setup.setName("default")
            setup.setEnabled(True)
            self.obj.add(setup)
            
        setup.setParameterSetID("ParameterSet/" + parsetName)
        
class ConfigModuleWrapper(object):
    def __init__(self, module):
        self.obj = module
        self.stations = {}

        for i in xrange(module.configStationCount()):
            station = module.configStation(i)
            self.stations[(station.networkCode(), station.stationCode())] = ConfigStationWrapper(station)

    def update(self, parsetName):
        if parsetName:
            self.obj.setParameterSetID("ParameterSet/" + parsetName)
        else:
            self.obj.setParameterSetID("")
    
    def removeStation(self, netCode, staCode):
        station = self.stations.get((netCode, staCode))
        if station:
            self.obj.remove(station.obj)
            del self.stations[(netCode, staCode)]
    
    def updateStation(self, netCode, staCode, parsetName):
        station = self.stations.get((netCode, staCode))
        if station is None:
            station = ConfigStationWrapper(DataModel.ConfigStation("Config/%s/%s/%s" % (self.obj.name(), netCode, staCode)))
            station.obj.setNetworkCode(netCode)
            station.obj.setStationCode(staCode)
            station.obj.setEnabled(True)
            self.obj.add(station.obj)
            self.stations[(netCode, staCode)] = station
        
        else:
            station.obj.update()
        
        station.update(parsetName)

class ConfigWrapper(object):
    def __init__(self, config):
        self.obj = config
        self.parsets = {}
        self.modules = {}

        for i in xrange(config.parameterSetCount()):
            parset = config.parameterSet(i)
            self.parsets[parset.publicID().split('/', 1)[1]] = ParsetWrapper(parset)
        
        for i in xrange(config.configModuleCount()):
            module = config.configModule(i)
            self.modules[module.name()] = ConfigModuleWrapper(module)

    def getModuleList(self):
        return self.modules.keys()
    
    def updateParameterSet(self, pkgName, parsetName, baseParsetName, kf):
        parset = self.parsets.get(parsetName)
        if parset is None:
            parset = ParsetWrapper(DataModel.ParameterSet("ParameterSet/" + parsetName))
            parset.obj.setCreated(Core.Time.GMT())
            self.obj.add(parset.obj)
            self.parsets[parsetName] = parset
        
        else:
            parset.obj.update()
        
        parset.update(pkgName, baseParsetName, kf)
        
    def updateModule(self, pkgName, parsetName):
        module = self.modules.get(pkgName)
        if module is None:
            module = ConfigModuleWrapper(DataModel.ConfigModule("Config/%s" % (pkgName,)))
            module.obj.setName(pkgName)
            module.obj.setEnabled(True)
            self.obj.add(module.obj)
            self.modules[pkgName] = module
        
        else:
            module.obj.update()
        
        module.update(parsetName)

    def removeStation(self, pkgName, netCode, staCode):
        self.modules[pkgName].removeStation(netCode, staCode)

    def updateStation(self, pkgName, netCode, staCode, parsetName):
        self.modules[pkgName].updateStation(netCode, staCode, parsetName)

class StreamWrapper(object):
    def __init__(self, stream):
        self.obj = stream

    def update(self, rate, rateDiv, orientation, datalogger, dataloggerId,
        seismometer, seismometerId, channel, depth, azimuth, dip,
        gainFreq, gainMult, gainUnit, format, restricted, inv):

        errmsg = None
        
        try:
            (smsn, smgain) = (seismometerId.split('/') + [None])[:2]
            (datalogger, dlgain, seismometer, smgain, gainFreq, gainUnit) = instdb.update_inventory(inv,
                datalogger, dataloggerId, gainMult, seismometer, smsn, smgain, rate, rateDiv)

        except NettabError, e:
            errmsg = str(e)
            dlgain = []
            smgain = []

        self.obj.setSampleRateNumerator(rate)
        self.obj.setSampleRateDenominator(rateDiv)
        self.obj.setDatalogger(datalogger)
        self.obj.setDataloggerSerialNumber(dataloggerId)
        self.obj.setDataloggerChannel(channel)
        self.obj.setSensor(seismometer)
        self.obj.setSensorSerialNumber(seismometerId)
        self.obj.setSensorChannel(channel)
        self.obj.setDepth(depth)
        self.obj.setAzimuth(azimuth)
        self.obj.setDip(dip)
        self.obj.setGainFrequency(gainFreq)
        self.obj.setGainUnit(gainUnit)
        self.obj.setFormat(format)
        self.obj.setFlags("GC")
        self.obj.setRestricted(restricted)
        self.obj.setShared(True)

        complete = True
        
        try:
            gain = smgain[channel] * dlgain[channel]

        except IndexError:
            complete = False
            gain = gainMult * getGain(datalogger, dataloggerId, seismometer,
                seismometerId, self.obj.code())

            if gain == 0 and errmsg:
                logs.error(errmsg)
        
        self.obj.setGain(gain)

        return complete
            
class StationWrapper(object):
    def __init__(self, netCode, station):
        self.obj = station
        self.netCode = netCode
        self.streams = {}
        self.__loc = {}
        self.__usedStreams = set()

        for i in xrange(station.sensorLocationCount()):
            loc = station.sensorLocation(i)
            self.__loc[loc.code()] = loc

            for j in xrange(loc.streamCount()):
                stream = loc.stream(j)
                try:
                    s = self.streams[(loc.code(), stream.code())]
                    if s.obj.start() > stream.start():
                        continue

                except KeyError:
                    pass
                
                self.streams[(loc.code(), stream.code())] = StreamWrapper(stream)
    
    def __updateStreams(self, sampling, orientation, datalogger, dataloggerId,
        seismometer, seismometerId, depth, gainFreq, gainMult, gainUnit,
        startDate, restricted, inv):

        complete = True
        channel = 0

        for (compCode, azimuth, dip) in parseOrientation(orientation):
            for (biCode, locCode, rate, rateDiv, format) in parseSampling(sampling):
                streamCode = biCode + compCode
                #(start, startString) = parseDate(startDate)
                start = self.obj.start()
                stream = self.streams.get((locCode, streamCode))
                if stream and stream.obj.start() < start:
                    stream.obj.setEnd(start)
                    stream.obj.update()
                    stream = None
                
                if stream is None:
                    loc = self.__loc.get(locCode)
                    if loc is None:
                        loc = DataModel.SensorLocation_Create()
                        loc.setCode(locCode)
                        loc.setStart(start)
                        self.obj.add(loc)
                        self.__loc[locCode] = loc

                    else:
                        loc.update()

                    loc.setLatitude(self.obj.latitude())
                    loc.setLongitude(self.obj.longitude())
                    loc.setElevation(self.obj.elevation())

                    stream = StreamWrapper(DataModel.Stream())
                    stream.obj.setCode(streamCode)
                    stream.obj.setStart(start)
                    loc.add(stream.obj)
                    self.streams[(locCode, streamCode)] = stream

                else:
                    stream.obj.update()
                    
                complete = stream.update(rate, rateDiv, orientation,
                    datalogger, dataloggerId, seismometer, seismometerId,
                    channel, depth, azimuth, dip, gainFreq, gainMult, gainUnit,
                    format, restricted, inv) and complete

                self.__usedStreams.add((locCode, streamCode))

            channel += 1

        return complete

    def update(self, DCID, restricted, kf, inv):
        self.obj.setDescription(kf.statDesc)
        self.obj.setLatitude(float(kf.latitude))
        self.obj.setLongitude(float(kf.longitude))
        self.obj.setElevation(float(kf.elevation))
        self.obj.setRestricted(restricted)
        self.obj.setShared(True)
        self.obj.setArchive(DCID)
        
        self.__usedStreams = set()

        complete = self.__updateStreams(kf.sampling1, kf.orientation1, kf.datalogger,
            kf.dataloggerSn, kf.seismometer1, kf.seismometerSn1, float(kf.depth1),
            0.02, float(kf.gainMult1), kf.unit1, kf.startDate, restricted, inv)
        
        if kf.sampling2:
            if not kf.depth2:
                logs.warning("missing depth of secondary sensor for %s %s" % (self.netCode, self.obj.code()))
                return

            if not hasattr(kf, "datalogger2") or not kf.datalogger2:
                kf.datalogger2 = kf.datalogger
            
            if not hasattr(kf, "dataloggerSn2") or not kf.dataloggerSn2:
                kf.dataloggerSn2 = kf.dataloggerSn
        
            complete = self.__updateStreams(kf.sampling2, kf.orientation2, kf.datalogger2,
                kf.dataloggerSn2, kf.seismometer2, kf.seismometerSn2, float(kf.depth2),
                1.0, float(kf.gainMult2), kf.unit2, kf.startDate, restricted, inv) and complete

        for (locCode, streamCode) in set(self.streams.keys()) - self.__usedStreams:
            self.__loc[locCode].remove(self.streams[(locCode, streamCode)].obj)
            del self.streams[(locCode, streamCode)]

        inv.flush()
        return complete
        
class NetworkWrapper(object):
    def __init__(self, network):
        self.obj = network
    
    def update(self, DCID, kf):
        self.obj.setDescription(kf.netDesc)
        self.obj.setArchive(DCID)

class InventoryWrapper(object):
    def __init__(self, inventory, DCID):
        self.obj = inventory
        self.inv = Inventory(inventory)
        self.DCID = DCID
        self.networks = {}
        self.stations = {}
       
        for i in xrange(inventory.networkCount()):
            network = inventory.network(i)
            self.networks[network.code()] = NetworkWrapper(network)

            for j in xrange(network.stationCount()):
                station = network.station(j)
                self.stations[(network.code(), station.code())] = StationWrapper(network.code(), station)

        self.inv.load_instruments()

    def updateNetwork(self, netCode, kf):
        network = self.networks.get(netCode)
        if network is None:
            network = NetworkWrapper(DataModel.Network("Network/%s" % (netCode,)))
            network.obj.setCode(netCode)
            network.obj.setStart(Core.Time(1980, 1, 1, 0, 0, 0))
            network.obj.setArchive(self.DCID)
            network.obj.setType("BB")
            network.obj.setNetClass("p")
            network.obj.setRestricted(False)
            network.obj.setShared(True)
            self.obj.add(network.obj)
            self.networks[netCode] = network
            
        else:
            network.obj.update()
        
        network.update(self.DCID, kf)
    
    def updateStation(self, netCode, staCode, restricted, kf):
        station = self.stations.get((netCode, staCode))
        (start, startString) = parseDate(kf.startDate)
        if station and station.obj.start() < start:
            for i in xrange(station.obj.sensorLocationCount()):
                loc = station.obj.sensorLocation(i)
                for j in xrange(loc.streamCount()):
                    try:
                        loc.stream(j).end()
                        continue
                    except Core.ValueException:
                        loc.stream(j).setEnd(start)
                        loc.stream(j).update()
                
                try:
                    station.obj.sensorLocation(i).end()
                    continue
                except Core.ValueException:
                    station.obj.sensorLocation(i).setEnd(start)
                    station.obj.sensorLocation(i).update()

            station.obj.setEnd(start)
            station.obj.update()
            station = None

        if station is None:
            station = StationWrapper(netCode, DataModel.Station("Station/%s/%s/%s" % (netCode, staCode, startString)))
            station.obj.setCode(staCode)
            station.obj.setStart(start)
            self.networks[netCode].obj.add(station.obj)
            self.stations[(netCode, staCode)] = station
            
        else:
            station.obj.update()
        
        return station.update(self.DCID, restricted, kf, self.inv)
            
    def setNetworkRestricted(self, netCode, state):
        network = self.networks.get(netCode)
        if network.obj.restricted() != state:
            network.obj.setRestricted(state)
            network.obj.update()

    def setStationRestricted(self, netCode, staCode, state):
        station = self.stations.get((netCode, staCode))
        try:
            if station.obj.restricted() != state:
                station.obj.setRestricted(state)
                station.obj.update()
        except:
            station.obj.setRestricted(state)
            station.obj.update()

        for stream in station.streams.itervalues():
            try:
                if stream.obj.restricted() != state:
                    stream.obj.setRestricted(state)
                    stream.obj.update()
            except:
                stream.obj.setRestricted(state)
                stream.obj.update()

class RouteWrapper(object):
    def __init__(self, route):
        self.obj = route
        self.arclink = {}
        self.seedlink = {}

        for i in xrange(route.routeArclinkCount()):
            self.arclink[route.routeArclink(i).address()] = route.routeArclink(i)

        for i in xrange(route.routeSeedlinkCount()):
            self.seedlink[route.routeSeedlink(i).address()] = route.routeSeedlink(i)

    def addArclink(self, addr, prio):
        try:
            arclink = self.arclink[addr]
            arclink.update()

        except KeyError:
            arclink = DataModel.RouteArclink()
            self.obj.add(arclink)
            self.arclink[addr] = arclink

        arclink.setStart(Core.Time(1980, 1, 1, 0, 0, 0))
        arclink.setEnd(None)
        arclink.setAddress(addr)
        arclink.setPriority(prio)

    def removeArclink(self, addr):
        self.obj.remove(self.arclink[addr])
        del self.arclink[addr]
    
    def addSeedlink(self, addr, prio):
        try:
            seedlink = self.seedlink[addr]
            seedlink.update()

        except KeyError:
            seedlink = DataModel.RouteSeedlink()
            self.obj.add(seedlink)
            self.seedlink[addr] = seedlink

        seedlink.setAddress(addr)
        seedlink.setPriority(prio)

    def removeSeedlink(self, addr):
        self.obj.remove(self.seedlink[addr])
        del self.seedlink[addr]

class RoutingWrapper(object):
    def __init__(self, routing):
        self.obj = routing
        self.routes = {}
        self.access = {}
        self.__existingArclink = {}
        self.__existingSeedlink = {}

        for i in xrange(routing.routeCount()):
            r = routing.route(i)
            self.routes[(r.networkCode(), r.stationCode(), r.locationCode(), r.streamCode())] = RouteWrapper(r)

        for i in xrange(routing.accessCount()):
            a = routing.access(i)
            if (a.networkCode(), a.stationCode()) in self.access:
                self.access[(a.networkCode(), a.stationCode())][a.user()] = a
            else:
                self.access[(a.networkCode(), a.stationCode())] = { a.user(): a }

    def __updateRouting(self, netCode, staCode, orientation, sampling, routeArclink, routeSeedlink):
        for (compCode, azimuth, dip) in parseOrientation(orientation):
            for (biCode, locCode, rate, rateDiv, format) in parseSampling(sampling):
                strmCode = biCode + compCode
                for (addr, prio, (rtNet, rtSta, rtStrm, rtLoc)) in parseRoute(routeArclink):
                    if fnmatch.fnmatchcase(netCode, rtNet):
                        rtNet = netCode

                    elif rtNet:
                        continue

                    if fnmatch.fnmatchcase(staCode, rtSta):
                        rtSta = staCode

                    elif rtSta:
                        continue

                    if fnmatch.fnmatchcase(locCode, rtLoc):
                        rtLoc = locCode

                    elif rtLoc:
                        continue

                    if fnmatch.fnmatchcase(strmCode, rtStrm):
                        rtStrm = strmCode

                    elif rtStrm:
                        continue

                    try:
                        route = self.routes[(rtNet, rtSta, rtLoc, rtStrm)]
                        route.obj.update()

                    except KeyError:
                        route = RouteWrapper(DataModel.Route("Route/%s/%s/%s/%s" % (rtNet, rtSta, rtLoc, rtStrm)))
                        route.obj.setNetworkCode(rtNet)
                        route.obj.setStationCode(rtSta)
                        route.obj.setLocationCode(rtLoc)
                        route.obj.setStreamCode(rtStrm)
                        self.obj.add(route.obj)
                        self.routes[(rtNet, rtSta, rtLoc, rtStrm)] = route

                    route.addArclink(addr, prio)

                    try:
                        addr_set = self.__existingArclink[(rtNet, rtSta, rtLoc, rtStrm)]

                    except KeyError:
                        addr_set = set()
                        self.__existingArclink[(rtNet, rtSta, rtLoc, rtStrm)] = addr_set
                    
                    addr_set.add(addr)

                for (addr, prio, (rtNet, rtSta, rtStrm, rtLoc)) in parseRoute(routeSeedlink):
                    if fnmatch.fnmatchcase(netCode, rtNet):
                        rtNet = netCode

                    elif rtNet:
                        continue

                    if fnmatch.fnmatchcase(staCode, rtSta):
                        rtSta = staCode

                    elif rtSta:
                        continue

                    if fnmatch.fnmatchcase(locCode, rtLoc):
                        rtLoc = locCode

                    elif rtLoc:
                        continue

                    if fnmatch.fnmatchcase(strmCode, rtStrm):
                        rtStrm = strmCode

                    elif rtStrm:
                        continue

                    try:
                        route = self.routes[(rtNet, rtSta, rtLoc, rtStrm)]
                        route.obj.update()

                    except KeyError:
                        route = RouteWrapper(DataModel.Route("Route/%s/%s/%s/%s" % (rtNet, rtSta, rtLoc, rtStrm)))
                        route.obj.setNetworkCode(rtNet)
                        route.obj.setStationCode(rtSta)
                        route.obj.setLocationCode(rtLoc)
                        route.obj.setStreamCode(rtStrm)
                        self.obj.add(route.obj)
                        self.routes[(rtNet, rtSta, rtLoc, rtStrm)] = route

                    route.addSeedlink(addr, prio)

                    try:
                        addr_set = self.__existingSeedlink[(rtNet, rtSta, rtLoc, rtStrm)]

                    except KeyError:
                        addr_set = set()
                        self.__existingSeedlink[(rtNet, rtSta, rtLoc, rtStrm)] = addr_set
                    
                    addr_set.add(addr)

    def __updateAccess(self, netCode, staCode, kfAccess):
        users = self.access.get((netCode, staCode))
        if users is None:
            users = {}
            self.access[(netCode, staCode)] = users

        existingUsers = set(users.keys())
        currentUsers = set(kfAccess.split())
        for id in existingUsers - currentUsers:
            self.obj.remove(users[id])
            del self.access[(netCode, staCode)][id]

        for id in currentUsers - existingUsers:
            access = DataModel.Access()
            access.setNetworkCode(netCode)
            access.setStationCode(staCode)
            access.setStreamCode("")
            access.setLocationCode("")
            access.setUser(id)
            access.setStart(Core.Time(1980, 1, 1, 0, 0, 0))
            access.setEnd(None)
            self.obj.add(access)
            self.access[(netCode, staCode)][id] = access
            
    def updateStation(self, netCode, staCode, kf, kf1):
        self.__updateRouting(netCode, staCode, kf.orientation1, kf.sampling1, kf1.routeArclink, kf1.routeSeedlink)
        if kf.orientation2 and kf.sampling2:
            self.__updateRouting(netCode, staCode, kf.orientation2, kf.sampling2, kf1.routeArclink, kf1.routeSeedlink)
        self.__updateAccess(netCode, staCode, kf1.access)

    def cleanup(self):
        for ((rtNet, rtSta, rtLoc, rtStrm), route) in self.routes.items():
            try:
                addr_set = self.__existingArclink[(rtNet, rtSta, rtLoc, rtStrm)]

            except KeyError:
                addr_set = set()

            for addr in route.arclink.keys():
                if addr not in addr_set:
                    route.removeArclink(addr)

            try:
                addr_set = self.__existingSeedlink[(rtNet, rtSta, rtLoc, rtStrm)]

            except KeyError:
                addr_set = set()

            for addr in route.seedlink.keys():
                if addr not in addr_set:
                    route.removeSeedlink(addr)

            if not route.arclink and not route.seedlink:
                self.obj.remove(route.obj)
                del self.routes[(rtNet, rtSta, rtLoc, rtStrm)]

class Key2DB(Client.Application):
    def __init__(self, argc, argv):
        Client.Application.__init__(self, argc, argv)
    
        self.setLoggingToStdErr(True)
        
        self.setMessagingEnabled(True)
        self.setDatabaseEnabled(True, True)
    
        self.setAutoApplyNotifierEnabled(False)
        self.setInterpretNotifierEnabled(False)
    
        self.setPrimaryMessagingGroup("LISTENER_GROUP")

    def initConfiguration(self):
        if not Client.Application.initConfiguration(self):
            return false
 
        # force logging to stderr even if logging.file = 1
        self.setLoggingToStdErr(True)

        return True
        
    def send(self, *args):
        while not self.connection().send(*args):
            logs.warning("send failed, retrying")
            time.sleep(1)
    
    def send_notifiers(self, group):
        Nsize = DataModel.Notifier.Size()

        if Nsize > 0:
            logs.debug("trying to apply %d changes..." % Nsize)
        else:
            logs.debug("no changes to apply")
            return

        Nmsg = DataModel.Notifier.GetMessage(True)
            
        it = Nmsg.iter()
        msg = DataModel.NotifierMessage()

        maxmsg = 100
        sent = 0
        mcount = 0

        try:
            try:
                while it.get():
                    msg.attach(DataModel.Notifier_Cast(it.get()))
                    mcount += 1
                    if msg and mcount == maxmsg:
                        sent += mcount
                        logs.debug("sending message (%5.1f %%)" % (sent / float(Nsize) * 100.0))
                        self.send(group, msg)
                        msg.clear()
                        mcount = 0
                        self.sync("update-config")

                    it.next()
            except:
                pass
        finally:
            if msg:
                logs.debug("sending message (%5.1f %%)" % 100.0)
                self.send(group, msg)
                msg.clear()
                self.sync("update-config")

    def __load_file(self, func, file):
        if file and os.path.exists(file):
            logs.info("loading " + file)
            func(file)

    def run(self):
        try:
            seiscompRoot = os.getenv("SEISCOMP_ROOT")
            if not seiscompRoot:
                seiscompRoot = "/home/sysop/seiscomp"

            DCID = ""
            updateInventory = True
            networkRestricted = {}
            incompleteResponse = {}

            try:
                kf = Keyfile(seiscompRoot + "/trunk/key/global")
                DCID = kf.datacenter
                updateInventory = (kf.updateInventory.lower() != "no")
            except IOError, e:
                pass

            global instdb
            instdb = Instruments(DCID)
            
            self.__load_file(loadGains, seiscompRoot + "/config/gain.dlsv")
            
            # for backwards compatibility
            self.__load_file(loadGains, seiscompRoot + "/config/gain.tab.out")
            self.__load_file(loadGains, seiscompRoot + "/config/gain.tab")

            try:
                self.__load_file(instdb.load_db, seiscompRoot + "/resp/inst.db")
                self.__load_file(instdb.load_sensor_attr, seiscompRoot + "/resp/sensor_attr.csv")
                self.__load_file(instdb.load_datalogger_attr, seiscompRoot + "/resp/datalogger_attr.csv")

            except (IOError, NettabError), e:
                logs.error("fatal error: " + str(e))
                return False

            sc3wrap.dbQuery = self.query()
            inventory = InventoryWrapper(self.query().loadInventory(), DCID)
            configdb = ConfigWrapper(self.query().loadConfig())
            routingdb = RoutingWrapper(self.query().loadRouting())

            DataModel.Notifier.Enable()
            DataModel.Notifier.SetCheckEnabled(False)

            existingConfigModules = set()
            existingParsets = set()
            existingNetworks = set()
            existingStations = set()
            
            try:
                kf = Keyfile(seiscompRoot + "/key/global")
                rootParsetName = "Global"
                existingParsets.add(rootParsetName)
                configdb.updateParameterSet(None, rootParsetName, None, kf)
                
            except IOError, e:
                rootParsetName = None
            
            self.send_notifiers("CONFIG")

            for p in glob.glob(seiscompRoot + "/pkg/*"):
                logs.debug("processing " + p)
                pkgName = p.split("/")[-1].split("_", 1)[1]

                try:
                    kf = Keyfile(seiscompRoot + "/%s/key/global" % (pkgName,))
                    baseParsetName = "%s/Global" % (pkgName,)
                    existingParsets.add(baseParsetName)
                    configdb.updateParameterSet(pkgName, baseParsetName, rootParsetName, kf)
                    
                except IOError, e:
                    baseParsetName = rootParsetName
            
                existingConfigModules.add(pkgName)
                configdb.updateModule(pkgName, baseParsetName)

                self.send_notifiers("CONFIG")

                for f in glob.glob(seiscompRoot + "/%s/key/profile_*" % (pkgName,)):
                    try:
                        logs.debug("processing " + f)
                        profile = f.split("/profile_")[-1]
                        try:
                            kf = Keyfile(f)
                        except IOError, e:
                            logs.error(str(e))
                            continue
                        
                        parsetName = "%s/Profile/%s" % (pkgName, profile)
                        existingParsets.add(parsetName)
                        configdb.updateParameterSet(pkgName, parsetName, baseParsetName, kf)

                        self.send_notifiers("CONFIG")
                    
                    except ValueError, e:
                        logs.error("%s: %s" % (f, str(e)))

                for f in glob.glob(seiscompRoot + "/%s/key/station_*" % (pkgName,)):
                    try:
                        logs.debug("processing " + f)
                        (netCode, staCode) = f.split("/station_")[-1].split("_", 1)
                        try:
                            kf = Keyfile(f)
                        except IOError, e:
                            logs.error(str(e))
                            continue
                        
                        parsetName = "%s/Station/%s/%s" % (pkgName, netCode, staCode)
                        existingParsets.add(parsetName)
                        configdb.updateParameterSet(pkgName, parsetName, baseParsetName, kf)

                        self.send_notifiers("CONFIG")

                    except ValueError, e:
                        logs.error("%s: %s" % (f, str(e)))

            for f in glob.glob(seiscompRoot + "/key/network_*"):
                try:
                    logs.debug("processing " + f)
                    netCode = f.split("/network_")[-1]
                    try:
                        kf = Keyfile(f)
                    except IOError, e:
                        logs.error(str(e))
                        continue

                    existingNetworks.add(netCode)
                    networkRestricted[netCode] = False

                    if updateInventory or netCode not in inventory.networks:
                        inventory.updateNetwork(netCode, kf)

                    self.send_notifiers("INVENTORY")
                
                except ValueError, e:
                    logs.error("%s: %s" % (f, str(e)))

            for f in glob.glob(seiscompRoot + "/key/station_*"):
                try:
                    logs.debug("processing " + f)
                    (netCode, staCode) = f.split("/station_")[-1].split('_', 1)
                    try:
                        kf = Keyfile(f)
                    except IOError, e:
                        logs.error(str(e))
                        continue

                    if netCode not in existingNetworks:
                        logs.warning("network %s does not exist, ignoring station %s" % (netCode, staCode))
                        continue
                    
                    if not kf.latitude:
                        logs.warning("missing latitude for %s %s" % (netCode, staCode))
                        continue

                    if not kf.longitude:
                        logs.warning("missing longitude for %s %s" % (netCode, staCode))
                        continue

                    if not kf.elevation:
                        logs.warning("missing elevation for %s %s" % (netCode, staCode))
                        continue

                    if not kf.depth1:
                        logs.warning("missing depth of primary sensor for %s %s" % (netCode, staCode))
                        continue

                    if decimal.Decimal(kf.latitude) == decimal.Decimal("0.0") and \
                        decimal.Decimal(kf.longitude) == decimal.Decimal("0.0"):
                        logs.warning("missing coordinates for %s %s" % (netCode, staCode))
                        continue

                    if not hasattr(kf, "orientation1") or not kf.orientation1:
                        logs.warning("missing orientation of primary sensor for %s %s, using default" % (netCode, staCode))
                        kf.orientation1 = "Z 0.0 -90.0; N 0.0 0.0; E 90.0 0.0"

                    if not hasattr(kf, "orientation2"):
                        kf.orientation2 = ""

                    if not hasattr(kf, "unit1") or not kf.unit1:
                        #logs.warning("missing unit of primary sensor for %s %s, using M/S" % (netCode, staCode))
                        kf.unit1 = "M/S"
                    
                    if not hasattr(kf, "unit2"):
                        #logs.warning("missing unit of secondary sensor for %s %s, using M/S**2" % (netCode, staCode))
                        kf.unit2 = "M/S**2"
                    
                    existingStations.add((netCode, staCode))

                    profiles = parsePkgstr(kf.packages)
                    for pkgName in configdb.getModuleList():
                        try:
                            pro = profiles[pkgName]
                            if pro:
                                parsetName = "%s/Profile/%s" % (pkgName, pro)
                            else:
                                parsetName = "%s/Station/%s/%s" % (pkgName, netCode, staCode)
                                
                            if parsetName in existingParsets:
                                configdb.updateStation(pkgName, netCode, staCode, parsetName)
                            else:
                                logs.warning("parameter set %s does not exist" % (parsetName,))
                                configdb.removeStation(pkgName, netCode, staCode)
                                
                        except KeyError:
                            configdb.removeStation(pkgName, netCode, staCode)
                            
                    self.send_notifiers("CONFIG")
            
                    restricted = False
                    
                    try:
                        pro = profiles['arclink']

                        if pro:
                            f1 = seiscompRoot + "/arclink/key/profile_%s" % (pro,)
                        else:
                            f1 = seiscompRoot + "/arclink/key/station_%s_%s" % (netCode, staCode)

                        logs.debug("processing " + f1)
                        try:
                            kf1 = Keyfile(f1)
                        except IOError, e:
                            logs.error(str(e))
                            continue

                        routingdb.updateStation(netCode, staCode, kf, kf1)
                    
                        restricted = (kf1.access != "")
                        if restricted:
                            networkRestricted[netCode] = True
                            
                    except KeyError:
                        pass

                    self.send_notifiers("ROUTING")
            
                    if updateInventory or (netCode, staCode) not in inventory.stations:
                        if not inventory.updateStation(netCode, staCode, restricted, kf):
                            try:
                                incNet = incompleteResponse[netCode]

                            except KeyError:
                                incNet = set()
                                incompleteResponse[netCode] = incNet
                            
                            incNet.add(staCode)

                    else:
                        inventory.setStationRestricted(netCode, staCode, restricted)
                    
                    self.send_notifiers("INVENTORY")

                except ValueError, e:
                    logs.error("%s: %s" % (f, str(e)))

            for (netCode, restricted) in networkRestricted.iteritems():
                inventory.setNetworkRestricted(netCode, restricted)
            
            if updateInventory:
                for (netCode, network) in inventory.networks.iteritems():
                    if netCode not in existingNetworks:
                        logs.notice("deleting network %s from inventory" % (netCode,))
                        inventory.obj.remove(network.obj)

                for ((netCode, staCode), station) in inventory.stations.iteritems():
                    if netCode in existingNetworks and (netCode, staCode) not in existingStations:
                        logs.notice("deleting station %s_%s from inventory" % (netCode, staCode))
                        inventory.networks[netCode].obj.remove(station.obj)

            self.send_notifiers("INVENTORY")

            for (pkgName, module) in configdb.modules.iteritems():
                if pkgName not in existingConfigModules:
                    logs.notice("deleting config module %s" % (pkgName))
                    configdb.obj.remove(module.obj)
                    continue

                for ((netCode, staCode), station) in module.stations.iteritems():
                    if netCode not in existingNetworks or \
                        (netCode, staCode) not in existingStations:
                        logs.notice("deleting station %s_%s from module %s" % (netCode, staCode, pkgName))
                        module.obj.remove(station.obj)

            for (parsetName, parset) in configdb.parsets.iteritems():
                if parsetName not in existingParsets:
                    logs.notice("deleting parameter set %s" % (parsetName))
                    configdb.obj.remove(parset.obj)
            
            self.send_notifiers("CONFIG")

            routingdb.cleanup()
            self.send_notifiers("ROUTING")

            if incompleteResponse:
                logs.info("The following stations are missing full response data")
                logs.info("Use sync_dlsv if needed")

                #for netCode in sorted(incompleteResponse.keys()):
                #    logs.info("%s: %s" % (netCode, " ".join(sorted(list(incompleteResponse[netCode])))))
                tmpDict = sortDictionary(incompleteResponse)
                for netCode in tmpDict.keys():
                    tmpSortedList = list(tmpDict[netCode])
                    tmpSortedList.sort()
                    logs.info("%s: %s" % (netCode, " ".join(tmpSortedList)))
                    
        except Exception:
            logs.print_exc()

        return True

if __name__ == "__main__":
    logs.debug = Logging.debug
    logs.info = Logging.info
    logs.notice = Logging.notice
    logs.warning = Logging.warning
    logs.error = Logging.error
    app = Key2DB(len(sys.argv), sys.argv)
    sys.exit(app())

