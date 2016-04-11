SET(QC_TARGET scqc)

SET(
	QC_SOURCES
		qctool.cpp
		main.cpp
)

SET(
	QC_HEADERS
		qctool.h
)

SC_ADD_EXECUTABLE(QC ${QC_TARGET})
SC_LINK_LIBRARIES_INTERNAL(${QC_TARGET} client qc qcplugin)
