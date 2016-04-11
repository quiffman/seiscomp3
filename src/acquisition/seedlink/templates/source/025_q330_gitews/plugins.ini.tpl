* Generated at #date# - Do not edit!
* template: #template#

[Q330_#statid#]

station=#statid#
udpaddr=#srcaddr#
baseport=#srcport#
dataport=#slot#
hostport=#udpport#
serialnumber=#serial#
authcode=#auth#
statefile=#pkgroot#/status/#statid#.q330.cn
messages=SDUMP,LOGEXTRA,AUXMSG
statusinterval=60

[WS_#statid#]

* Settings for Reinhardt weather station (GITEWS)

* Station ID (network/station code is set in seedlink.ini)
station=#statid#

* Use the command 'serial_plugin -m' to find out which protocols are
* supported.
protocol=mws

* Serial port 
port=#comport2#

* Baud rate
bps=#baudrate2#

* Time interval in minutes when weather information is logged, 0 (default)
* means "disabled". Weather channels can be used independently of this
* option.
statusinterval=60

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
channel KI source_id=TE scale=100

* Indoor Humidity (%)
channel II source_id=FE scale=1

* Air Pressure (hPa * 10)
channel DI source_id=DR scale=10

[IO_#statid#]

* Settings for WAGO I/O Controller

* Station ID (network/station code is set in seedlink.ini)
station=#statid#

* Use the command 'serial_plugin -m' to find out which protocols are
* supported.
protocol=modbus

* Serial port 
port=tcp://#srcaddr2#:#srcport2#

* Baud rate
bps=0

* Time interval in minutes when status information is logged, 0 (default)
* means "disabled". Status channels can be used independently of this
* option.
statusinterval=60

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

* Strom 5A 12V System
channel SA source_id=#sid_a# realscale=0.183 realunit=mA precision=2

* Strom 5A 24V System
channel SB source_id=#sid_b# realscale=0.183 realunit=mA precision=2

* Strom 5A Reserve
channel SC source_id=#sid_c# realscale=0.183 realunit=mA precision=2

* Strom 5A Reserve
channel SD source_id=#sid_d# realscale=0.183 realunit=mA precision=2

* Spannung 30V DC Solarcontroller 1
channel SE source_id=#sid_e# realscale=0.000916 realunit=V precision=3

* Spannung 30V DC Solarcontroller 2
channel SF source_id=#sid_f# realscale=0.000916 realunit=V precision=3

* Spannung 30V DC Solarcontroller 3
channel SG source_id=#sid_g# realscale=0.000916 realunit=V precision=3

* Spannung 30V DC Solarcontroller 4
channel SH source_id=#sid_h# realscale=0.000916 realunit=V precision=3

* Spannung 30V DC Solarcontroller 5
channel SI source_id=#sid_i# realscale=0.000916 realunit=V precision=3

* Spannung 30V DC Solarcontroller 6
channel SJ source_id=#sid_j# realscale=0.000916 realunit=V precision=3

* Spannung 30V DC Solarcontroller 7
channel SK source_id=#sid_k# realscale=0.000916 realunit=V precision=3

* Spannung 30V DC Solarcontroller 8
channel SL source_id=#sid_l# realscale=0.000916 realunit=V precision=3

* Spannung 30V DC Diodenverknüpfung SC1+SC2
channel SM source_id=#sid_m# realscale=0.000916 realunit=V precision=3

* Spannung 30V DC Diodenverknüpfung SC3+SC8
channel SN source_id=#sid_n# realscale=0.000916 realunit=V precision=3

* Spannung 30V DC 12V VSAT (geschaltet) [Sri Lanka, Madagaskar]
channel SO source_id=#sid_o# realscale=0.000916 realunit=V precision=3

* Spannung 30V DC 24V Converter
channel SP source_id=#sid_p# realscale=0.000916 realunit=V precision=3

* Spannung 30V DC VSAT Router geschaltet 24V
channel SQ source_id=#sid_q# realscale=0.000916 realunit=V precision=3

* Spannung 30V DC Diodenverknüpfung SC1-SC4 [Sri Lanka]
channel SR source_id=#sid_r# realscale=0.000916 realunit=V precision=3

* Spannung 30V Reserve
channel SS source_id=#sid_s# realscale=0.000916 realunit=V precision=3

* Spannung 30V Reserve
channel ST source_id=#sid_t# realscale=0.000916 realunit=V precision=3

* Spannung 30V Reserve
channel SU source_id=#sid_u# realscale=0.000916 realunit=V precision=3

* Wiederstandsmessung Schwimmer
channel SV source_id=#sid_v# realscale=0.5 realunit=Ohm precision=3

* Wiederstandsmessung Türk
channel SW source_id=#sid_w# realscale=0.5 realunit=Ohm precision=3

* Wiederstandsmessung Reserve
channel SX source_id=#sid_x# realscale=0.5 realunit=Ohm precision=3

* Wiederstandsmessung Reserve
channel SY source_id=#sid_y# realscale=0.5 realunit=Ohm precision=3

* 230V Messung
channel SZ source_id=#sid_z# realscale=0.1 realunit=V precision=3

