* template: #template#
plugin Q330_#statid# cmd = "#pkgroot#/bin/q330_plugin #daemon_opt# -v -f #pkgroot#/config/plugins.ini"
             timeout = 3600
             start_retry = 60
             shutdown_wait = 10

plugin WS_#statid# cmd="#pkgroot#/bin/serial_plugin #daemon_opt# -v -f #pkgroot#/config/plugins.ini"
             timeout = 600
             start_retry = 60
             shutdown_wait = 10

plugin IO_#statid# cmd="#pkgroot#/bin/serial_plugin #daemon_opt# -v -f #pkgroot#/config/plugins.ini"
             timeout = 600
             start_retry = 60
             shutdown_wait = 10

