* template: #template#
plugin sadc_#statid# cmd="#pkgroot#/bin/serial_plugin #daemon_opt# -v -f #pkgroot#/config/plugins.ini"
             timeout = 600
             start_retry = 60
             shutdown_wait = 10

