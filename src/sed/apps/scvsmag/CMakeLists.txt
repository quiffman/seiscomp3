SET(PROG_TARGET scvsmag)

SET(
	PROG_SOURCES
		scvsmag.cpp
		util.cpp
		timeline.cpp
		equations.cpp
		qualitycontrol.cpp
		Vs30Mapping.cpp
		VsSiteCondition.cpp
		main.cpp
)

ADD_DEFINITIONS(-DVSSC)
SC_ADD_EXECUTABLE(PROG ${PROG_TARGET})
SC_LINK_LIBRARIES_INTERNAL(${PROG_TARGET} client datamodel_vs)

# scvsmaglog
SET(MAIN_PY scvsmaglog.py)
SET(VSMAGLOG_TARGET scvsmaglog)

SET(
    VSMAGLOG_FILES
        scvsmaglog.py
)

SC_ADD_PYTHON_EXECUTABLE(VSMAGLOG)

# UD interface
SET(MAIN_PY ud_interface.py)
SET(UDINTERFACE_TARGET ud_interface)

SET(
    UDINTERFACE_FILES
        ud_interface.py
)

SC_ADD_PYTHON_EXECUTABLE(UDINTERFACE)
