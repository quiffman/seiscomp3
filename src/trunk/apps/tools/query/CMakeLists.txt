SET(SCQUERY_TARGET scquery)

SET(
    SCQUERY_HEADERS
    dbquery.h
    dbconnection.h
)

SET(
    SCQUERY_SOURCES
    dbquery.cpp
    dbconnection.cpp
	scquery.cpp
)


SC_ADD_EXECUTABLE(SCQUERY ${SCQUERY_TARGET})
TARGET_LINK_LIBRARIES(${SCQUERY_TARGET} ${Boost_program_options_LIBRARIES})
SC_LINK_LIBRARIES_INTERNAL(${SCQUERY_TARGET} client)
