SET(
	_DIR_LIST
	mseedfifo_plugin
	chain_plugin
)

IF ( NOT MACOSX )
	SET(
		_DIR_LIST 
		${_DIR_LIST}
		serial_plugin
		sockplugin
		naqsplugin
		nrts_plugin
		q330plugin
		reftek_plugin
		ewexport_plugin
		ewexport_pasv_plugin
		scream_plugin
		scream2_plugin
		wave24_plugin
		mseedscan_plugin
		fs_plugin
		win_plugin
	) 
ENDIF ( NOT MACOSX )

SUBDIRS(${_DIR_LIST})
