* template: #template#
plugin wave24_#statid# cmd="#pkgroot#/bin/wave24_plugin -p #comport# -s #baudrate# -N #netid# -S #statid# -1 HHZ -2 HHN -3 HHE -f 100"
             timeout = 600
             start_retry = 60
             shutdown_wait = 10

