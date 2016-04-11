SUBDIRS(seiscomp3 plugins)

SET(EVENT_TARGET scevent)

SET(
	EVENT_SOURCES
		main.cpp
		eventtool.cpp
		eventinfo.cpp
		util.cpp
		constraints.cpp
)


SET(
	EVENT_HEADERS
		eventtool.h
		eventinfo.h
		config.h
		constraints.h
		util.h
)

INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

SC_ADD_EXECUTABLE(EVENT ${EVENT_TARGET})
SC_LINK_LIBRARIES_INTERNAL(${EVENT_TARGET} seismology client evplugin)
SC_LINK_LIBRARIES(${EVENT_TARGET} ${OPENSSL_LIBRARIES})
