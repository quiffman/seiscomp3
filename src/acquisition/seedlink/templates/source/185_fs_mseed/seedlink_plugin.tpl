* template: #template#
plugin fs_mseed cmd="#pkgroot#/bin/fs_plugin #daemon_opt# -v -f #pkgroot#/config/plugins.ini"
             timeout = 1200
             start_retry = 60
             shutdown_wait = 10

