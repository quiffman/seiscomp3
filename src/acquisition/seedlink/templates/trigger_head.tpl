* Generated at #date# - Do not edit!
* template: #template#
*
* This file configures the event detector, which is used to trigger
* data streams
*
* In general, the label can be anything and it is used to "name" a
* particular trigger. This name will appear in the trigger list file names,
* in the unique trigger id's etc. However, in order to send correct trigger
* events to chain_plugin, the label *must* mach the station ID (which is
* the same as station name in most cases).
*
* Any field not present in the individual trigger sections will be
* copied from the default section.
*
* The 'net' and 'sta' entries must not be missing in any section!!!

[default]
cha = HH?

* default is empty location code
loc = ""

* currently only 'bandpass' is implemented
* specify 'bandpass' fmin fmax order    (space-separated)
* order of 4 results in a 24 dB slope
filter   = "bandpass 0.7 2.0 4"

* gain in counts/(nm/sec) (STS-2)
gain     = 0.6
sta_len  = 2
lta_len  = 100
trig_on  = 3
trig_off = 1.0

