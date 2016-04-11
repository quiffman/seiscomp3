SET(OLK_TARGET scolk)

SET(
	OLK_SOURCES
		main.cpp
		olk.cpp
		locator.cpp
)


SET(
	OLK_HEADERS
		olk.h
		locator.h
)


INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

SC_ADD_EXECUTABLE(OLK ${OLK_TARGET})
SC_LINK_LIBRARIES_INTERNAL(${OLK_TARGET} processing client seismology)
