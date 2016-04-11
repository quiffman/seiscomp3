#-------------------------------------------------------------------------
SET(QCPLUGIN_HEADERS
		qcplugin.h
		qcconfig.h
		qcmessenger.h
		qcbuffer.h
)
SET(QCPLUGIN_SOURCES
		qcplugin.cpp
		qcconfig.cpp
		qcmessenger.cpp
		qcbuffer.cpp
)

SC_LIB_INSTALL_HEADERS(QCPLUGIN)

SC_ADD_LIBRARY(QCPLUGIN qcplugin)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
SC_LIB_LINK_LIBRARIES(qcplugin ${Boost_signals_LIBRARIES})
#-------------------------------------------------------------------------


#-------------------------------------------------------------------------
MACRO(MAKE_QCPLUGIN _plugin)
	SET(QCP_HEADERS ${_plugin}.h)
	SET(QCP_SOURCES ${_plugin}.cpp)
	SC_ADD_PLUGIN_LIBRARY(QCP ${_plugin} qc)
	SC_LINK_LIBRARIES_INTERNAL(${_plugin} qcplugin)
ENDMACRO(MAKE_QCPLUGIN)

MAKE_QCPLUGIN(qcplugin_delay)
MAKE_QCPLUGIN(qcplugin_offset)
MAKE_QCPLUGIN(qcplugin_rms)
MAKE_QCPLUGIN(qcplugin_latency)
MAKE_QCPLUGIN(qcplugin_timing)
MAKE_QCPLUGIN(qcplugin_availability)
MAKE_QCPLUGIN(qcplugin_overlap)
MAKE_QCPLUGIN(qcplugin_gap)
MAKE_QCPLUGIN(qcplugin_spike)
MAKE_QCPLUGIN(qcplugin_outage)
#-------------------------------------------------------------------------


#-------------------------------------------------------------------------
INCLUDE_DIRECTORIES(.)
SC_LIB_LINK_LIBRARIES_INTERNAL(qcplugin utils client qc)
#-------------------------------------------------------------------------
