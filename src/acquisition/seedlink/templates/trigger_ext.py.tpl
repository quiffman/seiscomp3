# Generated at #date# - Do not edit!
# template: #template#

import sys
import time
from seiscomp import trigger
from seiscomp import lib

CONFIG_FILE = "#pkgroot#/config/trigger.ini"

def log_print(s):
    print time.ctime() + " - trigger: " + s
    sys.stdout.flush()

lib.message = log_print

try:
    trigger.start(CONFIG_FILE, log_print)

except (IOError, trigger.TriggerError), e:
    log_print(str(e))
    sys.exit(1)

