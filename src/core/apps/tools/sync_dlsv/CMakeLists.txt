SET(DLSV_TARGET sync_dlsv)

SET(
	DLSV_SOURCES
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
	DLSV_HEADERS
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

SC_ADD_EXECUTABLE(DLSV ${DLSV_TARGET})
SC_LINK_LIBRARIES_INTERNAL(${DLSV_TARGET} client datamodel)

