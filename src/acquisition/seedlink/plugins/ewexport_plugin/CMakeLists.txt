SET(EWEXPORTPLUGIN_SOURCES
	ewexport_plugin.c
	network.c
	swap.c
	util.c
	trheadconv.c
	../plugin.c
)

INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(..)

SET(OPTIONS -D_REENTRANT)

ADD_DEFINITIONS(${OPTIONS})

ADD_EXECUTABLE(ewexport_plugin ${EWEXPORTPLUGIN_SOURCES})
TARGET_LINK_LIBRARIES(ewexport_plugin pthread)

INSTALL(TARGETS ewexport_plugin RUNTIME DESTINATION acquisition/bin)
