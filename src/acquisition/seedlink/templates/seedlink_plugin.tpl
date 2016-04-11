* template: #template#
plugin chain cmd="#pkgroot#/bin/chain_plugin #daemon_opt# -v -f #pkgroot#/config/#chain_xml#"
             timeout = 600
             start_retry = 60
             shutdown_wait = 15

