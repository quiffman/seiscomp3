* Generated at #date# - Do not edit!
* template: #template#

[digiserv]

* "Digiserv" is an instance of SeedLink used internally in the system. All
* Digiserv options are also valid for SeedLink.

* Organization and default network code
organization = "#organization#"
network = XX

* Lockfile path
lockfile = #pkgroot#/status/digiserv.pid

* Directory where Seedlink will store its disk buffers
filebase = #pkgroot#/digiserv
#SPROC#
#SPROC#* Paths to config files of StreamProcessor
#SPROC#filters = #pkgroot#/config/filters.fir
#SPROC#streams = #pkgroot#/config/streams.xml
#SPROC#
#SPROC#* Use Steim2 encoding by default
#SPROC#encoding = steim2

* List of trusted addresses
trusted = "127.0.0.0/8"

* Features we don't need in digiserv
stream_check = disabled
window_extraction = disabled
info = all

* Show requests in log file
request_log = enabled

* Give warning if an input channel has time gap larger than 10 us
proc_gap_warn = 10

* Flush streams if an input channel has time gap larger than 0.1 s
proc_gap_flush = 100000

* Maximum allowed deviation from the sequence number of oldest packet if
* packet with requested sequence number is not found. If seq_gap_limit is
* exceeded, data flow starts from the next packet coming in, otherwise
* from the oldest packet in buffer.
seq_gap_limit = 1000

* Digiserv's TCP port
port = #digiserv_port#

* Number of recent Mini-SEED packets to keep in memory
buffers = 100

* Number of temporary files to keep on disk
segments = 10

* Size of one segment in 512-byte blocks
segsize = 100

* Access only from localhost
access = "127.0.0.0/8"

* Do not limit either number of connections or connection speed
connections = 0
bytespersec = 0

* The first station is always available in uni-station mode (regardless
* of the "access" parameter). Add a dummy station, so uni-station mode
* cannot be used.

station .dummy access = 0.0.0.0 description = "#organization#"

