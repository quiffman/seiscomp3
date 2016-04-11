import re

#
# conditional functions
#
# >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
def _retr(val):
	if val is None:
		val = ''
	s = re.sub("^[?]$", "*", val)       # single ? --> *
	s = re.sub("[?]", ".", s)           # SN?? --> SN..
	s = re.sub("[+]", ".+", s)          # + --> .+
	s = re.sub("[\*]", ".*", "^"+s+"$") # S*AA --> S.*AA
	return s


def _inPattern(code, pat):
	return re.match(_retr(pat), code)
	

def _inTimeSpan(obj, start_time, end_time):
	start = True
	end = True
	
	if (obj.start is not None) and (end_time is not None):
		start = (obj.start <= end_time)
	
	if (obj.end is not None) and (start_time is not None):
		end = (obj.end >= start_time)
	
	return (start and end)
	

def _passRestricted(obj, restricted):
	if (obj.restricted == None) or (restricted == None):
		return True
	
	return (obj.restricted == restricted)
	

def _passPermanent(netClass, permanent):
	if (netClass == None) or (permanent == None):
		return True
	
	if permanent == True:
		return (netClass == "p")
	elif permanent == False:
		return (netClass == "t")
	

def _inRegion(obj, latmin, latmax, lonmin, lonmax):
	minlat = True
	maxlat = True
	minlon = True
	maxlon = True

	if (latmin is not None):
		minlat = (obj.latitude >= latmin)
	if (latmax is not None):
		maxlat = (obj.latitude <= latmax)
	if (lonmin is not None):
		minlon = (obj.longitude >= lonmin)
	if (lonmax is not None):
		maxlon = (obj.longitude <= lonmax)
	if (lonmin is not None and lonmax is not None and lonmin > lonmax):
		minlon = maxlon = minlon or maxlon
	
	return (minlat and maxlat and minlon and maxlon)
	

def _passSensortype(obj, sensortype, sensor_dict):
	if (obj.sensor == None) or (sensortype == None):
		return True
	
	for st in sensortype.split("+"):
		try:
			if (sensor_dict[obj.sensor].type == st):
				return True

		except KeyError:
			pass

	return False


class _InventoryRule(object):
    def __init__(self, net_pat, sta_pat, chan_pat,
        loc_pat, start_time, end_time, sensortype,
        latmin, latmax, lonmin, lonmax,
        permanent, restricted,
        self_dcid, sync_dcid):
        self.net_pat = net_pat
        self.sta_pat = sta_pat
        self.chan_pat = chan_pat
        self.loc_pat = loc_pat
        self.start_time = start_time
        self.end_time = end_time
        self.sensortype = sensortype
        self.latmin = latmin
        self.latmax = latmax
        self.lonmin = lonmin
        self.lonmax = lonmax
        self.permanent = permanent
        self.restricted = restricted
        self.self_dcid = self_dcid
        self.sync_dcid = sync_dcid

class _StreamFilter(object):
    def __init__(self, rules, sensor_dict):
        self.__rules = rules
        self.__sensor_dict = sensor_dict

    def match(self, obj):
        for r in self.__rules:
            if _inPattern(obj.code, r.chan_pat) and \
               _inTimeSpan(obj, r.start_time, r.end_time) and \
               _passRestricted(obj, r.restricted) and \
               (not hasattr(obj, "sensor") or _passSensortype(obj, r.sensortype, self.__sensor_dict)):
               return (None, True)

        return (None, False)

class _LocationFilter(object):
    def __init__(self, rules, sensor_dict):
        self.__rules = rules
        self.__sensor_dict = sensor_dict

    def match(self, obj):
        match = False
        result = []
        for r in self.__rules:
            if _inPattern(obj.code, r.loc_pat):
               match = True
               if r.chan_pat is not None:
                   result.append(r)

        if not result:
            return (None, match)

        return (_StreamFilter(result, self.__sensor_dict), match)

class _StationFilter(object):
    def __init__(self, rules, sensor_dict):
        self.__rules = rules
        self.__sensor_dict = sensor_dict

    def match(self, obj):
        match = False
        result = []
        for r in self.__rules:
            if _inPattern(obj.code, r.sta_pat) and \
               _inRegion(obj, r.latmin, r.latmax, r.lonmin, r.lonmax) and \
               _inTimeSpan(obj, r.start_time, r.end_time) and \
               _passRestricted(obj, r.restricted) and \
               (not r.self_dcid or (obj.shared and r.self_dcid != obj.archive)) and \
               (not r.sync_dcid or (obj.shared and r.sync_dcid == obj.archive)):
               match = True
               if r.loc_pat is not None:
                   result.append(r)

        if not result:
            return (None, match)

        return (_LocationFilter(result, self.__sensor_dict), match)

class _NetworkFilter(object):
    def __init__(self, rules, sensor_dict):
        self.__rules = rules
        self.__sensor_dict = sensor_dict

    def match(self, obj):
        match = False
        result = []
        for r in self.__rules:
            if _inPattern(obj.code, r.net_pat) and \
               _inTimeSpan(obj, r.start_time, r.end_time) and \
               _passRestricted(obj, r.restricted) and \
               _passPermanent(obj, r.permanent):
               match = True
               if r.sta_pat is not None:
                   result.append(r)

        if not result:
            return (None, match)

        return (_StationFilter(result, self.__sensor_dict), match)

class InventoryFilter(object):
    def __init__(self, sensor_dict):
        self.__sensor_dict = sensor_dict
        self.__rules = []

    def add_rule(self, net_pat, sta_pat, chan_pat,
        loc_pat, start_time, end_time, sensortype,
        latmin, latmax, lonmin, lonmax,
        permanent, restricted,
        self_dcid, sync_dcid):
        self.__rules.append(_InventoryRule(net_pat, sta_pat, chan_pat,
            loc_pat, start_time, end_time, sensortype,
            latmin, latmax, lonmin, lonmax,
            permanent, restricted,
            self_dcid, sync_dcid))

    def network_filter(self):
        return _NetworkFilter(self.__rules, self.__sensor_dict)


class _RoutingRule(object):
    def __init__(self, net_pat, sta_pat, chan_pat,
        loc_pat, start_time, end_time):
        self.net_pat = net_pat
        self.sta_pat = sta_pat
        self.chan_pat = chan_pat
        self.loc_pat = loc_pat
        self.start_time = start_time
        self.end_time = end_time

class _RArclinkFilter(object):
    def __init__(self, rules):
        self.__rules = rules

    def match(self, obj):
        for r in self.__rules:
            if _inTimeSpan(obj, r.start_time, r.end_time):
                return (None, True)

        return (None, False)

class _RStreamFilter(object):
    def __init__(self, rules):
        self.__rules = rules

    def match(self, code):
        match = False
        result = []
        for r in self.__rules:
            if code == "" or _inPattern(code, r.chan_pat):
                match = True
                result.append(r)

        return (_RArclinkFilter(result), match)

class _RLocationFilter(object):
    def __init__(self, rules):
        self.__rules = rules

    def match(self, code):
        match = False
        result = []
        for r in self.__rules:
            if code == "" or _inPattern(code, r.loc_pat):
                match = True
                result.append(r)

        return (_RStreamFilter(result), match)

class _RStationFilter(object):
    def __init__(self, rules):
        self.__rules = rules

    def match(self, code):
        match = False
        result = []
        for r in self.__rules:
            if code == "" or _inPattern(code, r.sta_pat):
                match = True
                result.append(r)

        return (_RLocationFilter(result), match)

class _RNetworkFilter(object):
    def __init__(self, rules):
        self.__rules = rules

    def match(self, code):
        match = False
        result = []
        for r in self.__rules:
            if code == "" or _inPattern(code, r.net_pat):
                match = True
                result.append(r)

        return (_RStationFilter(result), match)

class RoutingFilter(object):
    def __init__(self):
        self.__rules = []

    def add_rule(self, net_pat, sta_pat, chan_pat,
        loc_pat, start_time, end_time):
        self.__rules.append(_RoutingRule(net_pat, sta_pat, chan_pat,
            loc_pat, start_time, end_time))

    def network_filter(self):
        return _RNetworkFilter(self.__rules)


