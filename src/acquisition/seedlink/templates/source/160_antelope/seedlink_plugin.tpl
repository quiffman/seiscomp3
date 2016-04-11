* template: #template#
plugin ORB_#statid# cmd="#pkgroot#/bin/orb_plugin -v -S #pkgroot#/status/#statid#.pkt:100 -m '.*#station#.*' -o #srcaddr#:#srcport#"
             timeout = 600
             start_retry = 60
             shutdown_wait = 10

