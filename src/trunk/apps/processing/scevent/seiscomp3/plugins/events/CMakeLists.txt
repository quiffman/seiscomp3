SET(EVPLUGIN_HEADERS
	eventprocessor.h
)

SET(EVPLUGIN_SOURCES
	eventprocessor.cpp
)

# Install the headers in install/seiscomp3/plugins/events
SC_LIB_INSTALL_HEADERS(EVPLUGIN seiscomp3/plugins/events)
SC_ADD_LIBRARY(EVPLUGIN evplugin)
SC_LIB_LINK_LIBRARIES_INTERNAL(evplugin core datamodel config)
