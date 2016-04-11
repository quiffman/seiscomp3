SET(PACKAGE_NAME CORE_GEO)

SET (
	${PACKAGE_NAME}_SOURCES
		geofeature.cpp
		geofeatureset.cpp
)

SET(
	${PACKAGE_NAME}_HEADERS
		geofeature.h
		geofeatureset.h
		rtree.h
)

SC_ADD_LIBRARY(${PACKAGE_NAME} coregeo)
SC_LIB_LINK_LIBRARIES_INTERNAL(coregeo config math)
SC_LIB_LINK_LIBRARIES(coregeo ${Boost_filesystem_LIBRARIES})
SC_LIB_INSTALL_HEADERS(${PACKAGE_NAME})

