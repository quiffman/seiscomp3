SET(DLSV2INV_TARGET dlsv2inv)

SET(
	DLSV2INV_SOURCES
	dataless.cpp
	inventory.cpp
	log.cpp
	main.cpp
	mystring.cpp
	seed.cpp
	synchro.cpp
	tmanip.cpp
)


SET(
	DLSV2INV_HEADERS
	dataless.h
	define.h
	exception.h
	inventory.h
	log.h
	mystring.h
	seed.h
	synchro.h
	tmanip.h
)

INCLUDE_DIRECTORIES(.)

SC_ADD_EXECUTABLE(DLSV2INV ${DLSV2INV_TARGET})
SC_LINK_LIBRARIES_INTERNAL(${DLSV2INV_TARGET} client datamodel)

