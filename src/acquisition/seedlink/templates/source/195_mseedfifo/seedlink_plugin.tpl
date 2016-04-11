* template: #template#
plugin mseedfifo cmd="#pkgroot#/bin/mseedfifo_plugin #daemon_opt# -v -d #pkgroot#/mseedfifo"
             timeout = 0
             start_retry = 1
             shutdown_wait = 10

