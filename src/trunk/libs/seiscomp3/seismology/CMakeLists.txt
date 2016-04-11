SET(SEISMOLOGY_SOURCES
	mb.cpp
	ML.cpp
	Mwp.cpp
	Mjma.cpp
	ttt.cpp

	locsat.cpp
        locatorinterface.cpp
        locsat_internal.cpp
)

SET(SEISMOLOGY_HEADERS
	ttt.h
	regions.h
	magnitudes.h
	locsat.h
        locatorinterface.h
)

SC_ADD_SUBDIR_SOURCES(SEISMOLOGY regions)
SC_ADD_SUBDIR_SOURCES(SEISMOLOGY ttt)
SC_SETUP_LIB_SUBDIR(SEISMOLOGY)
