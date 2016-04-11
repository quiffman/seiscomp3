SET(SEEDLINK_SOURCES
	seedlink.cc
	iosystem.cc
	monitor.cc
	sproc.cc
	mseed.cc
	steim1.cc
	steim2.cc
)

INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${SC3_PACKAGE_SOURCE_DIR}/common/libs/seiscomp/slutils)
INCLUDE_DIRECTORIES(../../libs/seiscomp/plugin)
INCLUDE_DIRECTORIES(../../libs/3rd-party/libslink)
INCLUDE_DIRECTORIES(../../libs/3rd-party/qlib2)


SET(CONFIG_FILE ${SC3_PACKAGE_INSTALL_PREFIX}/acquisition/config/seedlink.ini)

SET(OPTIONS -DSYSLOG_FACILITY=LOG_LOCAL0 -DCONFIG_FILE=\\\"${CONFIG_FILE}\\\" -DMSEED_ROUND_TENTH_MILLISEC)

# Uncomment the following to enable file descriptor reallocation.
# This is needed for large servers (number of stations multiplied by
# the number of simultaneous connections is larger than 1000).
SET(OPTIONS ${OPTIONS} -DFD_REALLOC)

ADD_DEFINITIONS(${OPTIONS})

ADD_EXECUTABLE(seedlink ${SEEDLINK_SOURCES})

TARGET_LINK_LIBRARIES(
	seedlink
		${LIBXML2_LIBRARIES}
		slink
		slutils
		slplugin
		qlib2
)

INSTALL(TARGETS seedlink
	RUNTIME DESTINATION acquisition/bin
	ARCHIVE DESTINATION acquisition/lib
	LIBRARY DESTINATION acquisition/lib
)
