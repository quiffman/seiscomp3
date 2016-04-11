* template: $template
plugin $seedlink.source.id cmd="$seedlink.plugin_dir/nmxptool -H $sources.nmxp.address -P $sources.nmxp.port -S -1 -M $sources.nmxp.max_latency -C '*.???' -k"
             timeout = 0
             start_retry = 60
             shutdown_wait = 10

