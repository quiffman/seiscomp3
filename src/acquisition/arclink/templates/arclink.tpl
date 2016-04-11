* Generated at #date# - Do not edit!
* template: #template#

[arclink]

organization = "#organization#"
request_dir = "#pkgroot#/requests"
dcid = "#dcid#"
contact_email = "#contactemail#"
connections = 500
connections_per_ip = 20
request_queue = 500
request_size = 1000
handlers_soft = 4
handlers_hard = 10
#SY#handler_cmd = "python #pkgroot#/lib/reqhandler.py -s"
#NS#handler_cmd = "python #pkgroot#/lib/reqhandler.py"
handler_timeout = 10
handler_start_retry = 60
handler_shutdown_wait = 10
port = #arclink_port#
lockfile = "#pkgroot#/status/arclink.pid"
statefile = "#pkgroot#/status/arclink.state"
admin_password = "#adminpass#"

* Maximum number of simultaneous request handler instances per request type
handlers_waveform = 2
handlers_response = 2
handlers_inventory = 2
handlers_routing = 2
handlers_qc = 2
handlers_greensfunc = 1

* Delete requests from RAM after 600 seconds of inactivity
swapout_time = 600

* Delete requests (and data products) completely after 10 days of inactivity
purge_time = 864000

* Enable the use of encryption to deliver restricted network data volumes
encryption = #encryption#

* File containing a list of users(emails) and passwords separated by ":"
password_file = "#pkgroot#/status/password.txt"
