SUBDIRS(share)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})


IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.generated)
	INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.generated)
ENDIF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.generated)


SET(
	CORE_DATAMODEL_SOURCES
		exchange/trunk.cpp
		exchange/scdm051.cpp
		${CORE_DATAMODEL_GENERATED_SOURCES}
		databasearchive.cpp
		messages.cpp
		notifier.cpp
		object.cpp
		publicobjectcache.cpp
		publicobject.cpp
		utils.cpp
)

SET(
	CORE_DATAMODEL_HEADERS
		databasearchive.h
		messages.h
		metadata.h
		notifier.h
		object.h
		publicobjectcache.h
		publicobject.h
		utils.h
		${CORE_DATAMODEL_GENERATED_HEADERS}
)

SC_ADD_LIBRARY(CORE_DATAMODEL datamodel)
SC_LIB_LINK_LIBRARIES_INTERNAL(datamodel io utils)
SC_LIB_LINK_LIBRARIES(datamodel ${Boost_thread_LIBRARIES})
SC_LIB_INSTALL_HEADERS(CORE_DATAMODEL)
