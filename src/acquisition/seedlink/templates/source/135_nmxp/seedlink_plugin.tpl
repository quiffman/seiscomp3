* template: #template#
plugin #pluginid# cmd="#pkgroot#/bin/nmxptool -H #srcaddr# -P #srcport# -S -1 -M 300 -C '*.???' -k"
             timeout = 600
             start_retry = 60
             shutdown_wait = 10

