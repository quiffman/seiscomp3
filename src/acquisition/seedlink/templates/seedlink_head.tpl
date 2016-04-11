* Generated at #date# - Do not edit!
* template: #template#

[seedlink]

* Organization and default network code
organization = "#organization#"
network = XX

* Lockfile path
lockfile = #pkgroot#/status/seedlink.pid

* Directory where Seedlink will store its disk buffers
filebase = #pkgroot#/seedlink
#SPROC#
#SPROC#* Paths to config files of StreamProcessor
#SPROC#filters = #pkgroot#/config/filters.fir
#SPROC#streams = #pkgroot#/config/streams.xml
#SPROC#
#SPROC#* Use Steim2 encoding by default
#SPROC#encoding = steim2

* List of trusted addresses
trusted = "127.0.0.0/8"

* Check start and end times of streams
stream_check = enabled

* If stream_check = enabled, also check for gaps in all channels that
* match given pattern. Register all gaps that are larger than +-0.5 seconds.
* gap_check_pattern = [EBLV][HLNG][ZNE]|S[NG][ZNE]
* Disabled to save memory.
gap_check_pattern = XXXXX
gap_treshold = 500000

* Disable window extraction from arbitrary Internet hosts
window_extraction = enabled

* Enable window extraction from trusted hosts
window_extraction_trusted = enabled

* INFO provided to arbitrary Internet hosts: ID, CAPABILITIES, STATIONS,
* STREAMS
info = streams

* INFO provided to trusted hosts: ID, CAPABILITIES, STATIONS, STREAMS,
* GAPS, CONNECTIONS, ALL
info_trusted = all

* Show requests in log file
request_log = enabled

* Give warning if an input channel has time gap larger than 10 us
proc_gap_warn = 10

* Flush streams if an input channel has time gap larger than 0.1 s
proc_gap_flush = 100000

* Reset FIR filters if an input channel has time gap larger than 1 s
proc_gap_reset = 1000000

* Maximum allowed deviation from the sequence number of oldest packet if
* packet with requested sequence number is not found. If seq_gap_limit is
* exceeded, data flow starts from the next packet coming in, otherwise
* from the oldest packet in buffer.
* Use the following to always start with the oldest packet:
* seq_gap_limit = 16777216
seq_gap_limit = 100000

* Server's TCP port
port = #seedlink_port#

* Number of recent Mini-SEED packets to keep in memory
buffers = 100

* Number of temporary files to keep on disk
segments = 10

* Size of one segment in 512-byte blocks
segsize = 1000

* Total number of TCP/IP connections allowed
connections = 500

* Maximum number of TCP/IP connections per IP
connections_per_ip = 20

* Maximum speed per connection (0: throttle disabled)
bytespersec = 0

* Defaults for all plugins. All of these parameters take value in seconds;
* zero disables the corresponding feature.
* 
* timeout -- shut down the plugin if no data arrives in this time period
*   [0 = wait for data forever];
* start_retry -- restart terminated plugins after this time period (plugins
*   can terminate themselves because of some internal error, or they can be
*   shut down by the Seedlink if timeout occurs or invalid data received)
*   [0 = don' restart terminated plugins];
* shutdown_wait -- wait this time period for a plugin to terminate after
*   sending the TERM signal. If a plugin will not terminate, it will be
*   terminated the hard way by sending the KILL signal [0 = wait forever].
plugin_timeout = 0
plugin_start_retry = 60
plugin_shutdown_wait = 10

* The first station is always available in uni-station mode (regardless
* of the "access" parameter). Add a dummy station, so uni-station mode
* cannot be used.

station .dummy access = 0.0.0.0 description = "#organization#"

* List of plugins which supply data to the Seedlink server. Several
* instances of a plugin under different names can be used, eg.
*   plugin edata1   cmd="#pkgroot#/bin/serial_plugin -v" timeout=600
*   plugin edata2   cmd="#pkgroot#/bin/serial_plugin -v" timeout=600
* In this case, also corresponding "edata1" and "edata2" sections in the
* plugins.ini file will have to be defined. The plugin parameters above can
* be set individually for each plugin here (without the "plugin_" suffix).
#RTSIM#
#RTSIM#plugin mseedfifo cmd="#pkgroot#/bin/mseedfifo_plugin #daemon_opt# -v -d #pkgroot#/mseedfifo"
#RTSIM#             timeout = 0
#RTSIM#             start_retry = 1
#RTSIM#             shutdown_wait = 10

