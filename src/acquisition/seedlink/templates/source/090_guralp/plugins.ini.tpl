* Generated at #date# - Do not edit!
* template: #template#

[guralp_#statid#]

* Settings for Guralp DM24 unit

* Station ID (network/station code is set in seedlink.ini)
station=#statid#

* Use the command 'serial_plugin -m' to find out which protocols are
* supported.
protocol=guralp2

* Serial port name, such as /dev/ttyS0
port=#comport#

* Baud rate
bps=#baudrate#

* The amount of microseconds to be added to the time reported by the
* digitizer.
time_offset=0

* Maximum number of consecutive zeros in datastream before data gap will be
* declared (-1 = disabled).
zero_sample_limit = -1

* Default timing quality in percents. This value will be used when no
* timing quality information is available. Can be -1 to omit the blockette
* 1001 altogether.
default_tq = 0

* Timing quality to use when GPS is out of lock
unlock_tq = 10

* Keyword 'channel' is used to map input channels to symbolic channel
* names. Use the 'gcfutil' program to find out the source IDs of your
* digitizer. Channel names are arbitrary 1..10-letter identifiers which
* should match the input names of the stream processing scheme in
* streams.xml, which is referenced from seedlink.ini

channel Z source_id=3704Z4
channel N source_id=3704N4
channel E source_id=3704E4

