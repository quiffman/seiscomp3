* template: #template#
plugin mseedscan cmd="#pkgroot#/bin/mseedscan_plugin -v -d #pkgroot#/indata -x #pkgroot#/status/mseedscan.seq"
             timeout = 1200
             start_retry = 60
             shutdown_wait = 10

