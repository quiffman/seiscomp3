#*****************************************************************************
# import_dlsv.py
#
# Generate keyfiles from (dataless) SEED volume
#
# (c) 2008 GFZ Potsdam
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any later
# version. For more information, see http://www.gnu.org/
#*****************************************************************************

import os
import fnmatch
import datetime
import optparse
import decimal
from seiscomp import logs
from seiscomp.keyfile import Keyfile

VERSION = "1.1 (2010.329)"
RECSIZE = 4096

# Allow leading and trailing spaces when converting to decimal
def Decimal(*args):
    if len(args) == 1 and isinstance(args[0], basestring):
        return decimal.Decimal(args[0].strip())
    else:
        return decimal.Decimal(*args)

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

def _inst_name(s):
    s1 = s.split()
    if len(s1) == 1:
        return s1[0].split(':')[0]
    elif len(s1) == 3:
        return s1[1].split('/')[0]
    else:
        return s1[1]

def _unicode_conv(s):
    s = " ".join(s.split()).replace("'", "\xc2\xb4")

    try:
        return s.decode("utf-8").encode("utf-8")

    except UnicodeDecodeError:
        return s.decode("iso-8859-1", "replace").encode("utf-8")

class PatternTable(object):
    def __init__(self, nkey, nval):
        self.__nkey = nkey
        self.__nval = nval
        self.__tab = {}

    def load(self, fileName):
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

                cols = line.split(None, abs(self.__nkey) + self.__nval - 1)
                if self.__nkey < 0:
                    key = tuple(cols[self.__nval:])
                    val = tuple(cols[:self.__nval])
                else:
                    key = tuple(cols[:self.__nkey])
                    val = tuple((cols[self.__nkey:] + self.__nval * ["",])[:self.__nval])

                if key in self.__tab:
                    self.__tab[key].append(val)
                else:
                    self.__tab[key] = [val]
                        
                line = fd.readline()

        finally:
            fd.close()

    def get(self, *args):
        if self.__nkey < 0:
            key = tuple(args[self.__nkey:])
            i1 = 0
            i2 = len(args) + self.__nkey
            i3 = self.__nval
            
        else:
            key = tuple(args[:self.__nkey])
            i1 = self.__nkey
            i2 = len(args)
            i3 = self.__nval + self.__nkey
            
        ret = []

        try:
            for pat in self.__tab[key]:
                for i in range(i1, i2):
                    if not fnmatch.fnmatch(args[i], pat[i-i1]):
                        break

                else:
                    ret.append(pat[i2-i1:i3-i1])
    
        except KeyError, e:
            pass

        return ret
        
class BlocketteReader(object):
    def __init__(self, data):
        self.__data = data
        self.__pos = 0
        self.__limit = len(self.__data)

    def setpos(self, pos):
        if pos > len(self.__data) or self.__data[pos:] == \
            (len(self.__data) - pos) * ' ':
            self.__pos = len(self.__data)

        else:
            self.__pos = pos

        return self.__pos

    def setlimit(self, limit):
        if limit is None:
            self.__limit = len(self.__data)
        
        else:
            self.__limit = min(len(self.__data), self.__pos + limit)

    def read(self, size):
        if self.__pos + size > self.__limit:
            if self.__pos != self.__limit:
                logs.error("unexpected end of blockette")

            self.__pos = self.__limit
            return None
        
        try:
            return self.__data[self.__pos:self.__pos+size]

        finally:
            self.__pos += size

    def varread(self, maxsize):
        maxstring = self.__data[self.__pos:self.__pos+maxsize+1]
        end = maxstring.find("~")
        if end == -1:
            logs.error("cannot find terminator of variable-length field")
            self.__pos += len(maxstring)
            return maxstring

        self.__pos += (end + 1)
        return maxstring[0:end]

    def readdate(self):
        dstr = self.varread(22)
        if len(dstr) == 0:
            return None

        d = (dstr.replace(':', ',').replace('.', ',').split(',') + 5 * [None])[:5]

        for i in xrange(len(d)):
            if d[i] is None:
                d[i] = [1980,1,0,0,0][i]
            else:
                d[i] = int(d[i])

        d2 = [d[0]] + list(_dy2mdy(d[1], d[0])) + d[2:]
        
        return datetime.datetime(*d2)
        
class DatalessReader(object):
    def __init__(self, inventory):
        self.__inv = inventory
        self.__saved_data = ""
        self.__abbreviation = {}
        self.__unit = {}
        self.__net = None
        self.__sta = None
        self.__ignore_chan = True
        self.__proc_blk = {
            33: self.__proc_blk33,
            34: self.__proc_blk34,
            50: self.__proc_blk50,
            52: self.__proc_blk52,
            53: self.__proc_blk53,
            58: self.__proc_blk58,
        }

    def __proc_blk33(self, p):
        key = int(p.read(3))
        desc = p.varread(50)
        self.__abbreviation[key] = desc
    
    def __proc_blk34(self, p):
        key = int(p.read(3))
        unit_name = p.varread(20)
        self.__unit[key] = unit_name
    
    def __proc_blk50(self, p):
        stat_code = p.read(5).strip()
        latitude = Decimal(p.read(10))
        longitude = Decimal(p.read(11))
        elevation = Decimal(p.read(7))
        p.read(4) # number of channels
        p.read(3) # number of station comments
        site_name = _unicode_conv(p.varread(60))
        network_key = int(p.read(3))
        p.read(4) # 32 bit word order
        p.read(2) # 16 bit word order
        start_date = p.readdate()
        end_date = p.readdate()
        p.read(1) # update flag
        network_code = p.read(2).strip()

        try:
            network_desc = _unicode_conv(self.__abbreviation[network_key])

        except KeyError:
            logs.error("Cannot find network in abbreviation dictionary")
            network_desc = ""
        
        try:
            self.__net = self.__inv.network[network_code]

        except KeyError:
            self.__net = self.__inv.add_network(network_code, network_desc)
            print "+ network", network_desc
        
        try:
            self.__sta = self.__net.station[stat_code]
            if start_date <= self.__sta.start_date:
                return

        except KeyError:
            print "  + station", site_name

        self.__sta = self.__net.add_station(stat_code, latitude, longitude,
            elevation, site_name, start_date, end_date)

    def __proc_blk52(self, p):
        loc_id = p.read(2).strip()
        chan_id = p.read(3).strip()
        p.read(4) # subchannel identifier
        instr_key = int(p.read(3))
        p.varread(30) # optional comment
        signal_units_key = int(p.read(3))
        p.read(3) # calibration units
        p.read(10) # latitude
        p.read(11) # longitude
        p.read(7) # elevation
        local_depth = Decimal(p.read(5))
        azimuth = Decimal(p.read(5))
        dip = Decimal(p.read(5))
        p.read(4) # data format identifier
        p.read(2) # data record length
        sample_rate = Decimal(p.read(10))
        p.read(10) # max clock drift
        p.read(4) # number of comments
        p.varread(26) # channel flags
        start_date = p.readdate()
        end_date = p.readdate()

        # hack to remove trailing zeros
        if sample_rate == int(sample_rate):
            sample_rate = Decimal(int(sample_rate))
        
        try:
            instr_name = _unicode_conv(self.__abbreviation[instr_key])

        except KeyError:
            logs.error("Cannot find instrument name in abbreviation dictionary")
            instr_name = ""
        
        if self.__sta and self.__sta.add_channel(loc_id, chan_id, local_depth, azimuth, dip, sample_rate, instr_name, start_date, end_date):

            self.__ignore_chan = False

        else:
            self.__ignore_chan = True
        
    def __proc_blk53(self, p):
        p.read(1) # transfer function type
        stage = int(p.read(2)) # stage sequence number
        unit_key = int(p.read(3))

        if stage != 1:
            return

        try:
            unit_name = self.__unit[unit_key]

        except KeyError:
            logs.error("Cannot find unit name in abbreviation dictionary")
            unit_name = ""

        if not self.__ignore_chan:
            self.__sta.set_unit(unit_name)

    def __proc_blk58(self, p):
        stage = int(p.read(2))
        gain = float(p.read(12))
        p.read(12) # freq

        if stage == 0 and not self.__ignore_chan:
            self.__sta.set_gain(gain)

    def __proc_record(self, buf):
        if buf[6] != 'A' and buf[6] != 'S':
            if self.__saved_data:
                logs.error("incomplete blockette")
                self.__saved_data = ""

            #logs.info("ignoring record of type " + buf[6])
            return

        if buf[7] != '*' and self.__saved_data:
            logs.error("incomplete blockette")
            self.__saved_data = ""
            
        buf1 = self.__saved_data + buf[8:]
        p = BlocketteReader(buf1)
        blk_start = 0

        while blk_start < len(buf1):
            p.setlimit(7)
            type = int(p.read(3))
            size = int(p.read(4))
            
            if blk_start + size > len(buf1):
                self.__saved_data = buf1[blk_start:]
                return

            p.setlimit(size)

            try:
                self.__proc_blk[type](p)

            except KeyError:
                #logs.warning("skipping unsupported blockette %s" % type)
                pass

            blk_start = p.setpos(blk_start + size)

        self.__saved_data = ""

    def read(self, file):
        fd = open(file)
        try:
            buf = fd.read(RECSIZE)
            while buf:
                self.__proc_record(buf)
                buf = fd.read(RECSIZE)

        finally:
            fd.close()

        # return self.__inv

class Instrument(object):
    def __init__(self, code, depth, name):
        self.code = code
        self.depth = depth
        self.name = name
        self.unit = ""
        self.stream = {}
        self.component = {}

    def __cmp__(self, other):
        return cmp(self.code, other.code)
    
    def add_stream(self, code, sample_rate):
        self.stream[code] = sample_rate

    def add_component(self, code, azimuth, dip):
        self.component[code] = (azimuth, dip)

    def sampling(self):
        s1 = []
        s2 = []
        if self.code[1]:
            s1.append("L" + self.code[1])

        if self.code[0] != "H":
            s1.append("T" + self.code[0])

        streams = self.stream.items()
        streams.sort(cmp=lambda a, b: cmp(a[1], b[1]), reverse=True)
        for (code, rate) in streams:
            c = ""
            if rate >= 80:
                if code != "H":
                    c = code
            elif rate >= 40:
                if code != "S":
                    c = code
            elif rate > 1:
                if code != "B":
                    c = code
            elif rate == 1:
                if code != "L":
                    c = code
            elif rate == Decimal("0.1"):
                if code != "V":
                    c = code
            elif rate == Decimal("0.01"):
                if code != "U":
                    c = code
            else:
                c = code

            s2.append(c + str(rate))

        if s1:
            return "/".join(s1) + "_" + "/".join(s2)

        return "/".join(s2)

    def orientation(self):
        s = []
        comp = self.component.copy()
        if "Z" in comp:
            s.append("Z %s %s" % tuple(comp["Z"]))
            del comp["Z"]

        if "N" in comp:
            s.append("N %s %s" % tuple(comp["N"]))
            del comp["N"]

        if "E" in comp:
            s.append("E %s %s" % tuple(comp["E"]))
            del comp["E"]

        comps = comp.items()
        comps.sort()
        for (code, (azimuth, dip)) in comps:
            s.append("%s %s %s" % (code, str(azimuth), str(dip)))

        return "; ".join(s)

    def reverse(self, code):
        (a, d) = self.component[code]
        if a < 180:
            self.component[code] = ((a + 180), -d)
        
        else:
            self.component[code] = ((a - 180), -d)

class Station(object):
    def __init__(self, code, latitude, longitude, elevation, description, start_date):
        self.code = code
        self.latitude = latitude
        self.longitude = longitude
        self.elevation = elevation
        self.description = description
        self.start_date = start_date
        self.instrument = {}
        self.gain = {}
        self.__last_chan = None
        self.channel_end_date = {}

    def __cmp__(self, other):
        return cmp(self.code, other.code)
    
    def add_channel(self, location, channel, depth, azimuth, dip, sample_rate, instrument_name, start_date, end_date):
        self.__last_chan = None

        try:
            if self.channel_end_date[(location, channel)] is None:
                return False

            if end_date is not None and \
                end_date <= self.channel_end_date[(location, channel)]:
                return False

        except KeyError:
            pass

        self.channel_end_date[(location, channel)] = end_date
        
        #if start_date > self.start_date:
        #    self.start_date = start_date
        
        code = (channel[1], location)
        instrument = self.instrument.get(code)
        if not instrument:
            instrument = Instrument(code, depth, instrument_name)
            self.instrument[code] = instrument

        instrument.add_stream(channel[0], sample_rate)
        instrument.add_component(channel[2], azimuth, dip)
        self.__last_chan = (channel, location)
        return True

    def set_gain(self, gain):
        if self.__last_chan:
            if gain < 0:
                gain = -gain
                self.instrument[(self.__last_chan[0][1], self.__last_chan[1])].reverse(self.__last_chan[0][2])

            self.gain[self.__last_chan] = gain

    def set_unit(self, unit):
        if self.__last_chan:
            self.instrument[(self.__last_chan[0][1], self.__last_chan[1])].unit = unit
 
class Network(object):
    def __init__(self, code, name):
        self.code = code
        self.name = name
        self.station = {}
    
    def __cmp__(self, other):
        return cmp(self.code, other.code)
    
    def add_station(self, code, latitude, longitude, elevation, description, start_date, end_date):
        station = Station(code, latitude, longitude, elevation, description, start_date)
        self.station[code] = station
        return station

class Inventory(object):
    def __init__(self):
        self.network = {}

    def add_network(self, code, name):
        network = Network(code, name)
        self.network[code] = network
        return network

t_primary = PatternTable(0, 4)
t_secondary = PatternTable(0, 4)
t_sensor1 = PatternTable(0, 4)
t_sensor2 = PatternTable(0, 4)
dlsv_gain = {}
seiscomp_root = None

def write_network(net):
    fd = open(seiscomp_root + "/key/network_%s" % (net.code,), "w")
    try:
        print >>fd, "KEY_VERSION='2.5'"
        print >>fd, "NET_NAME='%s'" % (net.code + "-Net",)
        print >>fd, "NET_DESC='%s'" % (net.name.strip(),)

    finally:
        fd.close()

def write_station(net, sta, packages):
    s1 = None
    for s in t_sensor1.get(net.code, sta.code):
        try:
            s1 = sta.instrument[s]
            break

        except KeyError:
            pass
    
    else:
        logs.warning("cannot find sensor1 for %s %s" % (net.code, sta.code))
        
    s2 = None
    for s in t_sensor2.get(net.code, sta.code):
        try:
            s2 = sta.instrument[s]
            break

        except KeyError:
            pass
    

    start_date_str =  "%d.%03d" % (sta.start_date.year, sta.start_date.timetuple()[7])
    dl_type = net.code + "." + sta.code + "." + start_date_str
    dl_serial = "xxxx"

    if s1:
        #s1_type = s1.name.strip().replace(" ", "_")
        #s1_serial = net.code + sta.code + s1.code[0] + s1.code[1]
        s1_type = net.code + "." + sta.code + "." + start_date_str + "." + s1.code[0] + s1.code[1]
        s1_serial = "yyyy"

    if s2:
        #s2_type = s2.name.strip().replace(" ", "_")
        #s2_serial = net.code + sta.code + s2.code[0] + s2.code[1]
        s2_type = net.code + "." + sta.code + "." + start_date_str + "." + s2.code[0] + s2.code[1]
        s2_serial = "yyyy"

    fd = open(seiscomp_root + "/key/station_%s_%s" % (net.code, sta.code), "w")
    try:
        print >>fd, "KEY_VERSION='2.5'"
        print >>fd, "STAT_DESC='%s'" % (sta.description.strip(),)
        print >>fd, "LATITUDE='%.4f'" % (sta.latitude,)
        print >>fd, "LONGITUDE='%.4f'" % (sta.longitude,)
        print >>fd, "ELEVATION='%.1f'" % (sta.elevation,)
        print >>fd, "DATALOGGER='%s'" % (dl_type,)
        print >>fd, "DATALOGGER_SN='%s'" % (dl_serial,)

        if s1:
            print >>fd, "SEISMOMETER1='%s'" % (s1_type,)
            print >>fd, "SEISMOMETER_SN1='%s'" % (s1_serial,)
            print >>fd, "GAIN_MULT1='1.0'"
            print >>fd, "UNIT1='%s'" % (s1.unit)
            print >>fd, "SAMPLING1='%s'" % (s1.sampling(),)
            print >>fd, "ORIENTATION1='%s'" % (s1.orientation(),)
            print >>fd, "DEPTH1='%.1f'" % (s1.depth,)
        else:
            print >>fd, "SEISMOMETER1=''"
            print >>fd, "SEISMOMETER_SN1=''"
            print >>fd, "GAIN_MULT1='1.0'"
            print >>fd, "UNIT1=''"
            print >>fd, "SAMPLING1=''"
            print >>fd, "ORIENTATION1=''"
            print >>fd, "DEPTH1='0.0'"

        if s2:
            print >>fd, "SEISMOMETER2='%s'" % (s2_type,)
            print >>fd, "SEISMOMETER_SN2='%s'" % (s2_serial,)
            print >>fd, "GAIN_MULT2='1.0'"
            print >>fd, "UNIT2='%s'" % (s2.unit)
            print >>fd, "SAMPLING2='%s'" % (s2.sampling(),)
            print >>fd, "ORIENTATION2='%s'" % (s2.orientation(),)
            print >>fd, "DEPTH2='%.1f'" % (s2.depth,)
        else:
            print >>fd, "SEISMOMETER2=''"
            print >>fd, "SEISMOMETER_SN2=''"
            print >>fd, "GAIN_MULT2='1.0'"
            print >>fd, "UNIT2=''"
            print >>fd, "SAMPLING2=''"
            print >>fd, "ORIENTATION2=''"
            print >>fd, "DEPTH2='0.0'"

        if sta.start_date.hour or sta.start_date.minute:
            if sta.start_date.second:
                print >>fd, "START_DATE='%d/%03d:%02d%02d%02d'" % (sta.start_date.year, sta.start_date.timetuple()[7], sta.start_date.hour, sta.start_date.minute, sta.start_date.second)
            else:
                print >>fd, "START_DATE='%d/%03d:%02d%02d'" % (sta.start_date.year, sta.start_date.timetuple()[7], sta.start_date.hour, sta.start_date.minute)
        else:
            print >>fd, "START_DATE='%d/%03d'" % (sta.start_date.year, sta.start_date.timetuple()[7])

        print >>fd, "CONFIGURED='yes'"
        print >>fd, "PACKAGES='%s'" % (packages,)

    finally:
        fd.close()

    for p in packages.split():
        if p.startswith("trunk:"):
            break

    else:
        pri_chan = ""
        pri_loc = ""
        sec_chan = ""
        sec_loc = ""

        if s1:
            for pri in t_primary.get(net.code, sta.code):
                if (pri[0][1], pri[1]) == s1.code and \
                    pri[0][0] in s1.stream and \
                    pri[0][2] in s1.component:

                    (pri_chan, pri_loc) = pri
                    break

            else:
                logs.warning("cannot find primary stream for %s %s" % (net.code, sta.code))
        
        if s2:
            for sec in t_secondary.get(net.code, sta.code):
                if (sec[0][1], sec[1]) == s2.code and \
                    sec[0][0] in s2.stream and \
                    sec[0][2] in s2.component:

                    (sec_chan, sec_loc) = sec
                    break

        detec_filter = "RMHP(10)>>ITAPER(30)>>BW(4,0.7,2)>>STALTA(2,80)"
        trig_on = "3"
        trig_off = "1.5"
        time_corr = "-0.8"
        focal_mechanism = ""
        equiv_net = ""
        equiv_streams = ""

        try:
            kf = Keyfile(seiscomp_root + "/trunk/key/station_%s_%s" % (net.code, sta.code))

            try: detec_filter = kf.detecFilter
            except AttributeError: pass

            try: trig_on = kf.trigOn
            except AttributeError: pass

            try: trig_off = kf.trigOff
            except AttributeError: pass

            try: time_corr = kf.timeCorr
            except AttributeError: pass

            try: focal_mechanism = kf.focalMechanism
            except AttributeError: pass

            try: equiv_net = kf.equivNet
            except AttributeError: pass

            try: equiv_streams = kf.equivStreams
            except AttributeError: pass

        except IOError:
            pass

        fd = open(seiscomp_root + "/trunk/key/station_%s_%s" % (net.code, sta.code), "w")
        try:
            print >>fd, "KEY_VERSION='2.5'"
            print >>fd, "DETEC_STREAM='%s'" % (pri_chan[:2],)
            print >>fd, "DETEC_LOCID='%s'" % (pri_loc,)
            print >>fd, "DETEC_STREAM2='%s'" % (sec_chan[:2],)
            print >>fd, "DETEC_LOCID2='%s'" % (sec_loc,)
            print >>fd, "DETEC_FILTER='%s'" % (detec_filter,)
            print >>fd, "TRIG_ON='%s'" % (trig_on,)
            print >>fd, "TRIG_OFF='%s'" % (trig_off,)
            print >>fd, "TIME_CORR='%s'" % (time_corr,)
            print >>fd, "FOCAL_MECHANISM='%s'" % (focal_mechanism,)
            print >>fd, "EQUIV_NET='%s'" % (equiv_net,)
            print >>fd, "EQUIV_STREAMS='%s'" % (equiv_streams,)
        
        finally:
            fd.close()

    for (cha, loc), gain in sta.gain.iteritems():
        if s1 and (cha[1], loc) == s1.code:
            dlsv_gain[(s1_type,s1_serial,cha)] = gain
        elif s2 and (cha[1], loc) == s2.code:
            dlsv_gain[(s2_type,s2_serial,cha)] = gain

    dlsv_gain[(dl_type,dl_serial,'*')] = 1.0
        
def parse_dataless(filename):
    inventory = Inventory()
    dr = DatalessReader(inventory)
    dr.read(filename)
    return inventory

def parse_command_line():
    parser = optparse.OptionParser(usage="usage: %prog [options]",
        version="%prog v" + VERSION)

    parser.set_defaults(packages="acquisition:geofon")
    
    parser.add_option("-p", "--packages", type="string", dest="packages",
        help="list of enabled packages (default %default)")
    
    (options, args) = parser.parse_args()

    if len(args) != 1:
        parser.error("incorrect number of arguments")
    
    return (args[0], options.packages)

def main():
    (filename, packages) = parse_command_line()

    global seiscomp_root
    seiscomp_root = os.getenv("SEISCOMP_ROOT")
    if not seiscomp_root:
        seiscomp_root = "/home/sysop/seiscomp"

    inventory = parse_dataless(filename)
    
    t_primary.load(seiscomp_root + "/config/primary.tab")
    t_sensor1.load(seiscomp_root + "/config/sensor1.tab")
    t_sensor2.load(seiscomp_root + "/config/sensor2.tab")

    try:
        t_secondary.load(seiscomp_root + "/config/secondary.tab")

    except IOError:
        pass

    try:
        fd = open(seiscomp_root + "/config/gain.dlsv")
        try:
            line = fd.readline()
            while line:
                (inst_type, serial, chan, gain) = line.split()
                dlsv_gain[(inst_type, serial, chan)] = float(gain)
                line = fd.readline()
    
        finally:
            fd.close()
    except IOError:
        pass

    if not os.path.exists(seiscomp_root + "/key"):
        os.makedirs(seiscomp_root + "/key")

    if not os.path.exists(seiscomp_root + "/trunk/key"):
        os.makedirs(seiscomp_root + "/trunk/key")
    
    for net in inventory.network.itervalues():
        write_network(net)

        for sta in net.station.itervalues():
            write_station(net, sta, packages)

    fd = open(seiscomp_root + "/config/gain.dlsv", "w")
    try:
        gain_list = dlsv_gain.items()
        gain_list.sort()
        for (inst_type, serial, chan), gain in gain_list:
            print >>fd, "%s %s %s %g" % (inst_type, serial, chan, gain)

    finally:
        fd.close()

if __name__ == "__main__":
    main()

