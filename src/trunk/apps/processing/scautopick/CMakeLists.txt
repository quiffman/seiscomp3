SET(PICK_TARGET scautopick)

SET(
	PICK_SOURCES
		main.cpp
		detector.cpp
		picker.cpp
		config.cpp
		stationconfig.cpp
)

SET(
	PICK_HEADERS
		detector.h
		picker.h
		config.h
		stationconfig.h
)

SET(
	PICK_DATA
		config/station.conf
)

INCLUDE_DIRECTORIES(.)

SC_ADD_EXECUTABLE(PICK ${PICK_TARGET})
SC_LINK_LIBRARIES_INTERNAL(${PICK_TARGET} processing)
SC_LINK_LIBRARIES(${PICK_TARGET} ${OPENSSL_LIBRARIES})
SC_INSTALL_DATA(PICK autopick)
