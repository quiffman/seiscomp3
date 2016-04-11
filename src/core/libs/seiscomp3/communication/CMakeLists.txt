SET(
	CORE_COMMUNICATION_SOURCES
		systemconnection.cpp
		connection.cpp
		protocol.cpp
		systemmessages.cpp
		networkinterface.cpp
		servicemessage.cpp
		connectioninfo.cpp
		clientstatus.cpp
		masterplugininterface.cpp
		spread/spreaddriver.cpp
)

SET(
	CORE_COMMUNICATION_HEADERS
		systemconnection.h
		connection.h
		protocol.h
		systemmessages.h
		networkinterface.h
		servicemessage.h
		masterplugininterface.h
		connectioninfo.h
		clientstatus.h
)

IF(NOT WIN32)
	SET(SPREAD_SOURCE_DIR ${SC3_PACKAGE_SOURCE_DIR}/libs/3rd-party/spread)
	SET(SPREAD_BINARY_DIR ${SC3_PACKAGE_BINARY_DIR}/libs/3rd-party/spread)
	SET(SPREAD_INC ${SPREAD_SOURCE_DIR}/include)
	SET(SPREAD_LIBRARIES ${SPREAD_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}tspread-core${CMAKE_SHARED_LIBRARY_SUFFIX})
	INCLUDE_DIRECTORIES(${SPREAD_INC})

	SET_SOURCE_FILES_PROPERTIES(
		spread/spreaddriver.cpp
		PROPERTIES OBJECT_DEPENDS ${SPREAD_LIBRARIES}
	)

	SET_DIRECTORY_PROPERTIES(
		PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${SPREAD_LIBRARIES}"
	)

	ADD_CUSTOM_COMMAND(
		OUTPUT ${SPREAD_LIBRARIES}
		COMMAND ./configure ARGS --prefix=${SPREAD_BINARY_DIR}
		COMMAND ${CMAKE_MAKE_PROGRAM}
		COMMAND ${CMAKE_MAKE_PROGRAM} ARGS install
		WORKING_DIRECTORY ${SPREAD_SOURCE_DIR}
	)
ENDIF(NOT WIN32)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${SPREAD_INCLUDE_DIR})

SC_ADD_LIBRARY(CORE_COMMUNICATION communication)
SC_LIB_LINK_LIBRARIES(communication ${Boost_iostreams_LIBRARIES} ${Boost_thread_LIBRARIES})
SC_LIB_LINK_LIBRARIES(communication ${SPREAD_LIBRARIES})

IF(WIN32)
	SC_LIB_LINK_LIBRARIES(communication zlib psapi)
ENDIF(WIN32)

SC_LIB_LINK_LIBRARIES_INTERNAL(communication config io core utils)
SC_LIB_INSTALL_HEADERS(CORE_COMMUNICATION)

IF(NOT WIN32)
        MAKE_DIRECTORY(${SPREAD_BINARY_DIR})
	INSTALL(
		DIRECTORY
			${SPREAD_BINARY_DIR}/
		DESTINATION
			${SC3_PACKAGE_INSTALL_PREFIX}
		USE_SOURCE_PERMISSIONS
		PATTERN "CMakeFiles" EXCLUDE
		PATTERN "*.cmake" EXCLUDE
		PATTERN "Makefile" EXCLUDE
	)
ENDIF(NOT WIN32)
