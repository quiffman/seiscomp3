* template: $template
plugin win$seedlink.win.id cmd="$seedlink.plugin_dir/win_plugin$seedlink._daemon_opt -v -F $seedlink.config_dir/config/win2sl${seedlink.win.id}.map $sources.win.udpport -"
             timeout = 3600
             start_retry = 60
             shutdown_wait = 10

