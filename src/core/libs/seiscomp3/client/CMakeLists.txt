SET(
	CORE_CLIENT_SOURCES
		application.cpp
		streamapplication.cpp
		commandline.cpp
		inventory.cpp
		configdb.cpp
		queue.cpp
		daemon.cpp
		pluginregistry.cpp
		monitor.cpp
)

SET(
	CORE_CLIENT_HEADERS
		application.h
		streamapplication.h
		commandline.h
		commandline.ipp
		queue.h
		queue.ipp
		inventory.h
		configdb.h
		daemon.h
		pluginregistry.h
		monitor.h
)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

SC_ADD_LIBRARY(CORE_CLIENT client)
SC_LIB_LINK_LIBRARIES(client ${Boost_program_options_LIBRARIES} ${Boost_thread_LIBRARIES})
SC_LIB_LINK_LIBRARIES_INTERNAL(client datamodel communication io utils seismology)
SC_LIB_INSTALL_HEADERS(CORE_CLIENT)
