SET(ARCLINK_SOURCES
	arclink.cc
	encrypt.cc
	encryptpasswordhandle.cc
)

SET(ARCLINK_HEADERS
	encrypt.h
	encrypterror.h
	encryptpasswordhandle.h
)

INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${SC3_PACKAGE_SOURCE_DIR}/common/libs/seiscomp/slutils)

SET(OPTIONS -DSYSLOG_FACILITY=LOG_LOCAL0)
ADD_DEFINITIONS(${OPTIONS})

ADD_EXECUTABLE(arclink ${ARCLINK_SOURCES})

TARGET_LINK_LIBRARIES(
	arclink
		${LIBXML2_LIBRARIES}
		slutils
		crypto
)

INSTALL(TARGETS arclink
	RUNTIME DESTINATION arclink/bin
	ARCHIVE DESTINATION arclink/lib
	LIBRARY DESTINATION arclink/lib
)
