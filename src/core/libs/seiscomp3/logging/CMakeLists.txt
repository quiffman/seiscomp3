SET(
	CORE_LOGGING_SOURCES
		log.cpp
		channel.cpp
		node.cpp
		publisher.cpp
		location.cpp
		init.cpp
		fd.cpp
		file.cpp
		filerotator.cpp
		output.cpp
)

SET(
	CORE_LOGGING_HEADERS
		common.h
		defs.h
		log.h
		channel.h
		node.h
		publisher.h
		publishloc.h
		location.h
		init.h
		output.h
		fd.h
		file.h
		filerotator.h
		syslog.h
)

IF(NOT WIN32)
	SET(CORE_LOGGING_SOURCES ${CORE_LOGGING_SOURCES} syslog.cpp)
	SET(CORE_LOGGING_HEADERS ${CORE_LOGGING_HEADERS} syslog.h
	)
ENDIF(NOT WIN32)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
ADD_DEFINITIONS("-DSEISCOMP_COMPONENT=\"log\"")

SC_ADD_LIBRARY(CORE_LOGGING log)
SC_LIB_LINK_LIBRARIES(log ${Boost_thread_LIBRARIES})
SC_LIB_INSTALL_HEADERS(CORE_LOGGING)
