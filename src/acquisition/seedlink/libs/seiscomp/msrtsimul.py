#!/usr/bin/env python

import sys, time, datetime, calendar
from   getopt import getopt, GetoptError
from seiscomp import mseedlite as mseed

ifile       = sys.stdin
verbosity   = 0
speed       = 1.
jump        = 0
test        = False

def rt_simul(f, speed=1., jump=0):
    """
    Iterator to simulate "real-time" MSeed input

    At startup, the first MSeed record is read. The following records are
    read in pseudo-real-time relative to the time of the first record,
    resulting in data flowing at realistic speed. This is useful e.g. for
    demonstrating real-time processing using real data of past events.

    The data in the input file may be multiplexed, but *must* be sorted by
    time, e.g. using 'mssort'.
    """
    import time

    rtime = time.time()
    stime = None
    skipping = True
    for rec in mseed.Input(f):
        rec_time = calendar.timegm(rec.begin_time.timetuple())
        if stime is None:
            stime = rec_time
        
        if skipping:
            if (rec_time - stime) / 60 < jump:
                continue
            
            stime = rec_time
            skipping = False
        
        tmax = stime + speed*(time.time() - rtime)
        if rec_time > tmax:
            time.sleep((rec_time-tmax+0.001)/speed)
        yield rec

usage_info = """
msrtsimul - read sorted (and possibly multiplexed) MiniSEED files and
        write the individual records in pseudo-real-time. This is useful
        e.g. for testing and simulating data acquisition.

Usage: msrtsimul.py [options] [file]

Options:
    -s, --speed         speed factor
    -j, --jump          number of minutes to skip
        --test          test mode
    -v, --verbose       verbose mode
    -h, --help          display this help message
"""

def usage(exitcode=0):
    sys.stderr.write(usage_info)
    sys.exit(exitcode)

try:
    opts, args = getopt(sys.argv[1:], "s:j:hv",
                        [ "speed=", "jump=", "test", "verbose", "help" ])
except GetoptError:
    usage(exitcode=1)

for flag, arg in opts:
    if   flag in ("-s", "--speed"):     speed = float(arg)
    elif flag in ("-j", "--jump"):      jump = int(arg)
    elif flag in ("-h", "--help"):      usage(exitcode=0)
    elif flag in ("-v", "--verbose"):   verbosity += 1
    elif flag in ("--test"):            test = True
    else: usage(exitcode=1)

if len(args) == 0:
    pass
elif len(args) == 1:
    fname = args[0]
    if fname != "-":
        ifile = file(fname)
else: usage(exitcode=1)

stime = time.time()
input = rt_simul(ifile, speed=speed, jump=jump)
time_diff = None
for rec in input:
    if time_diff is None:
        time_diff = datetime.datetime.utcnow() - rec.begin_time - \
            datetime.timedelta(microseconds = 1000000.0 * (rec.nsamp / rec.fsamp))

    rec.begin_time += time_diff

    if verbosity:
        sys.stderr.write("%s_%s %7.2f %s\n" % (rec.net, rec.sta, (time.time()-stime), str(rec.begin_time)))

    if not test:
        rec.write(sys.stdout, 9)
        sys.stdout.flush()

