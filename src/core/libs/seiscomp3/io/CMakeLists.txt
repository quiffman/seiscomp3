SUBDIRS(archive records recordstream gfarchive xml)

SET(
	CORE_IO_SOURCES
		database.cpp
		importer.cpp
		exporter.cpp
		recordinput.cpp
		recordstream.cpp
		recordoutputstream.cpp
		gfarchive.cpp
		socket.cpp

		archive/binarchive.cpp
		archive/xmlarchive.cpp

		records/binaryrecord.cpp
		records/xmlrecord.cpp
		records/shrecord.cpp
		records/sacrecord.cpp

		recordstream/file.cpp
		recordstream/memory.cpp
		recordstream/archive.cpp
		recordstream/sdsarchive.cpp
		recordstream/arclink.cpp
		recordstream/slconnection.cpp
		recordstream/combined.cpp
		recordstream/streamidx.cpp

		gfarchive/helmberger.cpp
		gfarchive/arclink.cpp
		gfarchive/saul.cpp

		xml/handler.cpp
		xml/importer.cpp
		xml/exporter.cpp
)

SET(
	CORE_IO_HEADERS
		database.h
		importer.h
		exporter.h
		recordinput.h
		recordstreamexceptions.h
		recordstream.h
		recordoutputstream.h
		gfarchive.h
		socket.h
)


IF (NOT WIN32)
	SET(
		CORE_IO_SOURCES
			${CORE_IO_SOURCES}
			records/ahrecord.cpp
	)
ENDIF (NOT WIN32)

IF (MSEED_FOUND)
	SET(
		CORE_IO_SOURCES
			${CORE_IO_SOURCES}
			records/mseedrecord.cpp
	)
	INCLUDE_DIRECTORIES(${LIBMSEED_INCLUDE_DIR})
	SET(HAVE_MSEED 1)
ENDIF (MSEED_FOUND)


INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${Boost_filesystem_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${Boost_regex_INCLUDE_DIRS})
ADD_DEFINITIONS(${LIBXML2_DEFINITIONS})

SC_ADD_LIBRARY(CORE_IO io)
SC_LIB_LINK_LIBRARIES_INTERNAL(io core log utils)

IF(WIN32)
	SC_LIB_LINK_LIBRARIES(io zlib)
ENDIF(WIN32)

SC_LIB_LINK_LIBRARIES(
	io
		${LIBXML2_LIBRARIES}
		${Boost_iostreams_LIBRARIES}
		${Boost_filesystem_LIBRARIES}
		${Boost_regex_LIBRARIES}
)

IF ( MACOSX )
    SC_LIB_LINK_LIBRARIES(io iconv)
ENDIF ( MACOSX )

IF (CMAKE_SYSTEM_NAME STREQUAL SunOS)
	SC_LIB_LINK_LIBRARIES(io iconv)
ENDIF (CMAKE_SYSTEM_NAME STREQUAL SunOS)

IF (MSEED_FOUND)
	SC_LIB_LINK_LIBRARIES(io ${LIBMSEED_LIBRARY})
ENDIF (MSEED_FOUND)

SC_LIB_INSTALL_HEADERS(CORE_IO)
