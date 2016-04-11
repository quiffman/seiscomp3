* Generated at #date# - Do not edit!
* template: #template#

[ED_#statid#]

* Settings for the Earth Data PS2400/PS6-24 digitizer (firmware >= 2.23)

* Station ID (network/station code is set in seedlink.ini)
station=#statid#

* Use the command 'serial_plugin -m' to find out which protocols are
* supported.
protocol=edata_r

* Port can be a serial port, pipe or "tcp://ip:port"
port=#comport#

* Baud rate is only meaningful for a serial port.
bps=#baudrate#

* lsb (defaults to 8): least significant bit (relative to 32-bit samples),
*   normally 8 for 24-bit samples, but can be set for example to 7 to get
*   25-bit samples;
* statusinterval (defaults to 0): time interval in minutes when "state of
*   health" information is logged, 0 means "disabled". State of health
*   channels can be used independently of this option.
*
* If you set 'checksum' to a wrong value then the driver will not work and
* you will get error messages like "bad SUM segment" or "bad MOD segment".
lsb=8
statusinterval=60

* Parameter 'time_offset' contains the amount of microseconds to be added
* to the time reported by the digitizer.

* 1.389 sec is possibly the correct offset if you have a version of the
* Earth Data digitizer with external GPS unit.
* time_offset=1389044

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
* names. Channel names are arbitrary 1..10-letter identifiers which should
* match the input names of the stream processing scheme in streams.xml,
* which is referenced from seedlink.ini

channel Z source_id=0
channel N source_id=1
channel E source_id=2

channel Z1 source_id=3
channel N1 source_id=4
channel E1 source_id=5

* "State of health" channels (1 sps) have source IDs 6...19.
* Which Mini-SEED streams (if any) are created from these channels
* depends on the stream processing scheme.

channel S1 source_id=6
channel S2 source_id=7
channel S3 source_id=8
channel S4 source_id=9
channel S5 source_id=10
channel S6 source_id=11
channel S7 source_id=12
channel S8 source_id=13

* Channel 20 records the phase difference between the incoming GPS 1pps
* signal and the internal one second mark. One unit is 333 us.

channel PLL source_id=20

[WS_#statid#]

* Settings for Lacrosse WS2300 weather station

* Station ID (network/station code is set in seedlink.ini)
station=#statid#

* Use the command 'serial_plugin -m' to find out which protocols are
* supported.
protocol=ws2300

* Serial port 
port=#comport#

* Baud rate (fixed at 2400)
bps=2400

* pctime (defaults to 0): use PC time instead of the internal clock of the
*   weather station (0: no, 1: yes);
* statusinterval (defaults to 0): time interval in minutes when weather information
*   is logged, 0 means "disabled". Weather channels can be used independently
*   of this option.
pctime=1
statusinterval=60

* Parameter 'time_offset' contains the amount of microseconds to be added
* to the time reported by the digitizer.

* 1.389 sec is possibly the correct offset if you have a version of the
* Earth Data digitizer with external GPS unit.
* time_offset=1389044

* Maximum number of consecutive zeros in datastream before data gap will be
* declared (-1 = disabled).
zero_sample_limit = -1

* Default timing quality in percents. This value will be used when no
* timing quality information is available. Can be -1 to omit the blockette
* 1001 altogether.
default_tq = -1

* Keyword 'channel' is used to map input channels to symbolic channel
* names. Channel names are arbitrary 1..10-letter identifiers which should
* match the input names of the stream processing scheme in streams.xml,
* which is referenced from seedlink.ini

* Indoor Temperature (C * 100)
channel KI source_id = 0

* Outdoor Temperature (C * 100) 
channel KO source_id = 1

* Windchill (C * 100)
channel KW source_id = 2

* Dewpoint (C * 100)
channel KD source_id = 3

* Indoor Humidity (%)
channel II source_id = 4

* Outdoor Humidity (%)
channel IO source_id = 5

* Total Rainfall (mm * 100)
channel RA source_id = 6

* Rainfall in 1h (mm * 100)
channel R1 source_id = 7

* Rainfall in 24h (mm * 100)
channel R2 source_id = 8

* Wind Speed (m/s * 10)
channel WS source_id = 9

* Wind Direction (deg / 22.5)
channel WD source_id = 10

* Absolute Air Pressure (hPa * 10)
channel DI source_id = 11

* Relative Air Pressure (hPa * 10)
* channel DI source_id = 12

* HF Status (4 bits)
channel HF source_id = 13

* Raw wind speed and direction data (24 bits)
* channel WR source_id = 14

