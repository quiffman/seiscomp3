SUBDIRS(
	platform
)

SET(BUILDVERSION_CPP ${CMAKE_CURRENT_BINARY_DIR}/version_build.cpp)

SET(
	CORE_CORE_SOURCES
		rtti.cpp
		metaobject.cpp
		datetime.cpp
		timewindow.cpp
		optional.cpp
		strings.cpp
		arrayfactory.cpp
		typedarray.cpp
		record.cpp
		array.cpp
		genericrecord.cpp
		greensfunction.cpp
		status.cpp
		recordsequence.cpp
		interruptible.cpp
		message.cpp
		genericmessage.cpp
		datamessage.cpp
		exceptions.cpp
		plugin.cpp
		system.cpp
		baseobject.cpp
		multicomponentarray.cpp
		version.cpp
		${BUILDVERSION_CPP}
)

SET(
	CORE_CORE_HEADERS
		archive.h
		archive.inl
		archive.ipp
		baseobject.h
		baseobject.inl
		defs.h
		factory.h
		factory.inl
		factory.ipp
		interfacefactory.h
		interfacefactory.ipp
		io.h
		rtti.h
		metaobject.h
		metaproperty.h
		serialization.h
		datetime.h
		timewindow.h
		optional.h
		optional.inl
		enumeration.h
		enumeration.inl
		strings.h
		strings.ipp
		arrayfactory.h
		array.h
		typedarray.h
		record.h
		genericrecord.h
		greensfunction.h
		exceptions.h
		status.h
		recordsequence.h
		interruptible.h
		message.h
		genericmessage.h
		genericmessage.ipp
		datamessage.h
		plugin.h
		system.h
		multicomponentarray.h
		multicomponentarray.inl
		version.h
)


IF(WIN32)
	SET(CORE_CORE_SOURCES ${CORE_SOURCES} strptime.c)
	SET(CORE_CORE_HEADERS ${CORE_HEADERS} strptime.h)
ENDIF(WIN32)


SET_SOURCE_FILES_PROPERTIES(${BUILDVERSION_CPP}
	PROPERTIES GENERATED true
)


IF (CMAKE_SIZEOF_VOID_P MATCHES "8")
	SET(BUILD_ARCH "x86_64")
ELSE(CMAKE_SIZEOF_VOID_P MATCHES "8")
	SET(BUILD_ARCH "i686")
ENDIF(CMAKE_SIZEOF_VOID_P MATCHES "8")


ADD_DEFINITIONS(-DWITH_BUILD_INFOS)
ADD_CUSTOM_COMMAND(
	OUTPUT  ${BUILDVERSION_CPP}
	COMMAND ${CMAKE_COMMAND} -E echo_append 'char const* build_system\(void\) {char const* BUILD_SYSTEM = \"' >> ${CMAKE_CURRENT_BINARY_DIR}/version_pre.cpp
	COMMAND ${CMAKE_COMMAND} -E echo_append `uname -sr` >> ${CMAKE_CURRENT_BINARY_DIR}/version_pre.cpp
	COMMAND ${CMAKE_COMMAND} -E echo '\"\; return BUILD_SYSTEM\;}' >> ${CMAKE_CURRENT_BINARY_DIR}/version_pre.cpp

	COMMAND ${CMAKE_COMMAND} -E echo_append 'char const* compiler_version\(void\) {char const* COMPILER_VERSION = \"' >> ${CMAKE_CURRENT_BINARY_DIR}/version_pre.cpp
	COMMAND ${CMAKE_COMMAND} -E echo_append `${CMAKE_CXX_COMPILER} --version | head -1` >> ${CMAKE_CURRENT_BINARY_DIR}/version_pre.cpp
	COMMAND ${CMAKE_COMMAND} -E echo '\"\; return COMPILER_VERSION\;}' >> ${CMAKE_CURRENT_BINARY_DIR}/version_pre.cpp

	COMMAND ${CMAKE_COMMAND} -E echo_append 'char const* os_version\(void\) {char const* OS_VERSION = \"' >> ${CMAKE_CURRENT_BINARY_DIR}/version_pre.cpp
	COMMAND ${CMAKE_COMMAND} -E echo_append `lsb_release -sd | sed 's/\"//g'` >> ${CMAKE_CURRENT_BINARY_DIR}/version_pre.cpp
	COMMAND ${CMAKE_COMMAND} -E echo_append '/' >> ${CMAKE_CURRENT_BINARY_DIR}/version_pre.cpp
	COMMAND ${CMAKE_COMMAND} -E echo_append '${BUILD_ARCH}' >> ${CMAKE_CURRENT_BINARY_DIR}/version_pre.cpp
	COMMAND ${CMAKE_COMMAND} -E echo '\"\; return OS_VERSION\;}' >> ${CMAKE_CURRENT_BINARY_DIR}/version_pre.cpp

	COMMAND ${CMAKE_COMMAND} -E copy_if_different  ${CMAKE_CURRENT_BINARY_DIR}/version_pre.cpp ${BUILDVERSION_CPP}
	COMMENT "create ${BUILDVERSION_CPP}"
)


INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})

SC_ADD_LIBRARY(CORE_CORE core)
SC_LIB_LINK_LIBRARIES_INTERNAL(core log)
SC_LIB_LINK_LIBRARIES(core ${Boost_thread_LIBRARIES})
IF(WIN32)
	SC_LIB_LINK_LIBRARIES(core ws2_32)
ENDIF(WIN32)
SC_LIB_INSTALL_HEADERS(CORE_CORE)
