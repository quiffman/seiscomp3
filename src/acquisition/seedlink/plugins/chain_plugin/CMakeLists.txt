SET(CHAINPLUGIN_SOURCES
	chain_plugin.cc
	schedule.c
)

INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${SC3_PACKAGE_SOURCE_DIR}/common/libs/seiscomp/slutils)
INCLUDE_DIRECTORIES(../../libs/seiscomp/plugin)
INCLUDE_DIRECTORIES(../../libs/3rd-party/libslink)
INCLUDE_DIRECTORIES(../../libs/3rd-party/qlib2)

SET(OPTIONS "-DSYSLOG_FACILITY=LOG_LOCAL0")

ADD_DEFINITIONS(${OPTIONS})

ADD_EXECUTABLE(chain_plugin ${CHAINPLUGIN_SOURCES})

TARGET_LINK_LIBRARIES(
	chain_plugin
		${LIBXML2_LIBRARIES}
		slink
		qlib2
		slplugin
		slutils
)

INSTALL(TARGETS chain_plugin RUNTIME DESTINATION acquisition/bin)
