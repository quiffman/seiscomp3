MACRO(SC_BEGIN_PACKAGE ...)
	SET(SC3_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
	SET(SC3_PACKAGE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
	IF (${ARGC} GREATER 1)
		SET(SC3_PACKAGE_DIR ${ARGV1})
		SET(SC3_PACKAGE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/${ARGV1})
		SET(SC3_PACKAGE_BIN_DIR ${ARGV1}/bin)
		SET(SC3_PACKAGE_LIB_DIR ${ARGV1}/lib)
		SET(SC3_PACKAGE_PYTHON_LIB_DIR ${ARGV1}/${PYTHON_LIBRARY_PATH})
		SET(SC3_PACKAGE_INCLUDE_DIR ${ARGV1}/include)
		SET(SC3_PACKAGE_SHARE_DIR ${ARGV1}/share)
		SET(SC3_PACKAGE_CONFIG_DIR ${ARGV1}/etc)
		SET(SC3_PACKAGE_TEMPLATES_DIR ${ARGV1}/templates)
	ELSE (${ARGC} GREATER 1)
		SET(SC3_PACKAGE_DIR "")
		SET(SC3_PACKAGE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
		SET(SC3_PACKAGE_BIN_DIR bin)
		SET(SC3_PACKAGE_LIB_DIR lib)
		SET(SC3_PACKAGE_PYTHON_LIB_DIR ${PYTHON_LIBRARY_PATH})
		SET(SC3_PACKAGE_INCLUDE_DIR include)
		SET(SC3_PACKAGE_SHARE_DIR share)
		SET(SC3_PACKAGE_CONFIG_DIR etc)
		SET(SC3_PACKAGE_TEMPLATES_DIR templates)
	ENDIF (${ARGC} GREATER 1)
	MESSAGE(STATUS "Adding pkg ${ARGV0}")
	MESSAGE(STATUS "... resides in ${SC3_PACKAGE_SOURCE_DIR}.")
	MESSAGE(STATUS "... will be installed under ${SC3_PACKAGE_INSTALL_PREFIX}.")
ENDMACRO(SC_BEGIN_PACKAGE)


MACRO(SC_ADD_LIBRARY _library_package _library_name)
	SET(_global_library_package SC_${_library_package})

	IF (SHARED_LIBRARIES)
		SET(${_library_package}_TYPE SHARED)
		SET(${_global_library_package}_SHARED 1)
		IF (WIN32)
			ADD_DEFINITIONS(-D${_global_library_package}_EXPORTS)
		ENDIF (WIN32)
	ENDIF (SHARED_LIBRARIES)

	SET(LIBRARY ${_global_library_package})
	SET(LIBRARY_NAME ${_library_name})

	IF (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake)
		CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake
		               ${CMAKE_CURRENT_BINARY_DIR}/config.h)
		CONFIGURE_FILE(${CMAKE_CURRENT_BINARY_DIR}/config.h
		               ${CMAKE_CURRENT_BINARY_DIR}/config.h)
		CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/win32api.h.cmake
		               ${CMAKE_CURRENT_BINARY_DIR}/api.h)
		SET(${_library_package}_HEADERS
			${${_library_package}_HEADERS}
			${CMAKE_CURRENT_BINARY_DIR}/api.h
			${CMAKE_CURRENT_BINARY_DIR}/config.h)
	ELSE (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake)
		CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/win32api.h.cmake
		               ${CMAKE_CURRENT_BINARY_DIR}/api.h)
		SET(${_library_package}_HEADERS
			${${_library_package}_HEADERS}
			${CMAKE_CURRENT_BINARY_DIR}/api.h)
	ENDIF (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake)

	ADD_LIBRARY(seiscomp3_${_library_name} ${${_library_package}_TYPE} ${${_library_package}_SOURCES})

	INSTALL(TARGETS seiscomp3_${_library_name}
		RUNTIME DESTINATION ${SC3_PACKAGE_BIN_DIR}
		ARCHIVE DESTINATION ${SC3_PACKAGE_LIB_DIR}
		LIBRARY DESTINATION ${SC3_PACKAGE_LIB_DIR}
	)
ENDMACRO(SC_ADD_LIBRARY)


MACRO(SC_ADD_PLUGIN_LIBRARY _library_package _library_name _plugin_app)
	SET(_global_library_package SEISCOMP3_PLUGIN_${_library_package})

	SET(${_global_library_package}_SHARED 1)

	ADD_LIBRARY(${_library_name} MODULE ${${_library_package}_SOURCES})
	SET_TARGET_PROPERTIES(${_library_name} PROPERTIES PREFIX "")

	SET(LIBRARY ${_global_library_package})
	SET(LIBRARY_NAME ${_library_name})

	INSTALL(TARGETS ${_library_name}
		DESTINATION ${SC3_PACKAGE_SHARE_DIR}/plugins/${_plugin_app}
	)
ENDMACRO(SC_ADD_PLUGIN_LIBRARY)


MACRO(SC_ADD_GUI_PLUGIN_LIBRARY _library_package _library_name _plugin_app)
	INCLUDE(${QT_USE_FILE})

	SET(_global_library_package SEISCOMP3_PLUGIN_${_library_package})

	SET(${_global_library_package}_SHARED 1)

	# Create MOC Files
	IF (${_library_package}_MOC_HEADERS)
		QT4_WRAP_CPP(${_library_package}_MOC_SOURCES ${${_library_package}_MOC_HEADERS})
	ENDIF (${_library_package}_MOC_HEADERS)

	# Create UI Headers
	IF (${_library_package}_UI)
		QT4_WRAP_UI(${_library_package}_UI_HEADERS ${${_library_package}_UI})
		INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
		INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
	ENDIF (${_library_package}_UI)

    # Add resources
	IF (${_library_package}_RESOURCES)
		QT4_ADD_RESOURCES(${_library_package}_RESOURCE_SOURCES ${${_library_package}_RESOURCES})
	ENDIF (${_library_package}_RESOURCES)

	SET(
		${_library_package}_FILES
			${${_library_package}_SOURCES}
			${${_library_package}_MOC_SOURCES}
			${${_library_package}_UI_HEADERS}
			${${_library_package}_RESOURCE_SOURCES}
	)

	ADD_LIBRARY(${_library_name} MODULE ${${_library_package}_FILES})
	SET_TARGET_PROPERTIES(${_library_name} PROPERTIES PREFIX "")
	TARGET_LINK_LIBRARIES(${_library_name} ${QT_LIBRARIES})

	SET(LIBRARY ${_global_library_package})
	SET(LIBRARY_NAME ${_library_name})

	INSTALL(TARGETS ${_library_name}
		DESTINATION ${SC3_PACKAGE_SHARE_DIR}/plugins/${_plugin_app}
	)
ENDMACRO(SC_ADD_GUI_PLUGIN_LIBRARY)


MACRO(SC_ADD_GUI_LIBRARY_CUSTOM_INSTALL _library_package _library_name)
	INCLUDE(${QT_USE_FILE})

	SET(_global_library_package SC_${_library_package})

	IF (SHARED_LIBRARIES)
		IF (WIN32)
			ADD_DEFINITIONS(-D${_global_library_package}_EXPORTS)
		ENDIF (WIN32)
		SET(${_library_package}_TYPE SHARED)
		SET(${_global_library_package}_SHARED 1)
	ENDIF (SHARED_LIBRARIES)

	# Create MOC Files
	IF (${_library_package}_MOC_HEADERS)
		QT4_WRAP_CPP(${_library_package}_MOC_SOURCES ${${_library_package}_MOC_HEADERS})
	ENDIF (${_library_package}_MOC_HEADERS)

	# Create UI Headers
	IF (${_library_package}_UI)
		QT4_WRAP_UI(${_library_package}_UI_HEADERS ${${_library_package}_UI})
		INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
		INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
	ENDIF (${_library_package}_UI)

    # Add resources
	IF (${_library_package}_RESOURCES)
		QT4_ADD_RESOURCES(${_library_package}_RESOURCE_SOURCES ${${_library_package}_RESOURCES})
	ENDIF (${_library_package}_RESOURCES)

	SET(LIBRARY ${_global_library_package})
	SET(LIBRARY_NAME ${_library_name})

	IF (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake)
		CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake
		               ${CMAKE_CURRENT_BINARY_DIR}/config.h)
		CONFIGURE_FILE(${CMAKE_CURRENT_BINARY_DIR}/config.h
		               ${CMAKE_CURRENT_BINARY_DIR}/config.h)
		CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/win32api.h.cmake
		               ${CMAKE_CURRENT_BINARY_DIR}/api.h)
		SET(${_library_package}_HEADERS
			${${_library_package}_HEADERS}
			${CMAKE_CURRENT_BINARY_DIR}/api.h
			${CMAKE_CURRENT_BINARY_DIR}/config.h)
	ELSE (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake)
		CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/win32api.h.cmake
		               ${CMAKE_CURRENT_BINARY_DIR}/api.h)
		SET(${_library_package}_HEADERS
			${${_library_package}_HEADERS}
			${CMAKE_CURRENT_BINARY_DIR}/api.h)
	ENDIF (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake)

	SET(
	${_library_package}_FILES_
	    ${${_library_package}_SOURCES}
	    ${${_library_package}_MOC_SOURCES}
	    ${${_library_package}_UI_HEADERS}
	    ${${_library_package}_RESOURCE_SOURCES}
	)

	ADD_LIBRARY(seiscomp3_${_library_name} ${${_library_package}_TYPE} ${${_library_package}_FILES_})
	TARGET_LINK_LIBRARIES(seiscomp3_${_library_name} ${QT_LIBRARIES})
ENDMACRO(SC_ADD_GUI_LIBRARY_CUSTOM_INSTALL)


MACRO(SC_ADD_GUI_LIBRARY _library_package _library_name)
	SC_ADD_GUI_LIBRARY_CUSTOM_INSTALL(${_library_package} ${_library_name})

	INSTALL(TARGETS seiscomp3_${_library_name}
		RUNTIME DESTINATION ${SC3_PACKAGE_BIN_DIR}
		ARCHIVE DESTINATION ${SC3_PACKAGE_LIB_DIR}
		LIBRARY DESTINATION ${SC3_PACKAGE_LIB_DIR}
	)
ENDMACRO(SC_ADD_GUI_LIBRARY)


MACRO(SC_ADD_EXECUTABLE _package _name)
	ADD_EXECUTABLE(${_name} ${${_package}_SOURCES})
	INSTALL(TARGETS ${_name}
		RUNTIME DESTINATION ${SC3_PACKAGE_BIN_DIR}
		ARCHIVE DESTINATION ${SC3_PACKAGE_LIB_DIR}
		LIBRARY DESTINATION ${SC3_PACKAGE_LIB_DIR}
	)

	IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config/${_name}.cfg)
		INSTALL(
			FILES config/${_name}.cfg
			DESTINATION ${SC3_PACKAGE_CONFIG_DIR}
		)
	ENDIF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config/${_name}.cfg)
ENDMACRO(SC_ADD_EXECUTABLE)


MACRO(SC_ADD_PYTHON_EXECUTABLE _name)
	FOREACH(file ${${_name}_FILES})
		IF(NOT MAIN_PY)
			SET(MAIN_PY ${file})
		ENDIF(NOT MAIN_PY)
	ENDFOREACH(file)

	CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/src/core/apps/python/run_wrapper.template
	               ${CMAKE_CURRENT_BINARY_DIR}/${${_name}_TARGET}
	               @ONLY)

	INSTALL(
		FILES ${${_name}_FILES}
		DESTINATION ${SC3_PACKAGE_BIN_DIR}
	)

	INSTALL(
		PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${${_name}_TARGET}
		DESTINATION ${SC3_PACKAGE_BIN_DIR}
	)

	IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config/${${_name}_TARGET}.cfg)
		INSTALL(
			FILES config/${${_name}_TARGET}.cfg
			DESTINATION ${SC3_PACKAGE_CONFIG_DIR}
		)
	ENDIF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config/${${_name}_TARGET}.cfg)
ENDMACRO(SC_ADD_PYTHON_EXECUTABLE)


MACRO(SC_ADD_TEST_EXECUTABLE _package _name)
	ADD_DEFINITIONS(-DSEISCOMP_TEST_DATA_DIR=\\\""${PROJECT_TEST_DATA_DIR}"\\\")
	ADD_EXECUTABLE(${_name} ${${_package}_SOURCES})
ENDMACRO(SC_ADD_TEST_EXECUTABLE)



MACRO(SC_ADD_GUI_EXECUTABLE _package _name)
	INCLUDE(${QT_USE_FILE})

	# Create MOC Files
	IF (${_package}_MOC_HEADERS)
		QT4_WRAP_CPP(${_package}_MOC_SOURCES ${${_package}_MOC_HEADERS})
	ENDIF (${_package}_MOC_HEADERS)

	# Create UI Headers
	IF (${_package}_UI)
		QT4_WRAP_UI(${_package}_UI_HEADERS ${${_package}_UI})
		INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
		INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
	ENDIF (${_package}_UI)

	# Add resources
	IF (${_package}_RESOURCES)
		QT4_ADD_RESOURCES(${_package}_RESOURCE_SOURCES ${${_package}_RESOURCES})
	ENDIF (${_package}_RESOURCES)

	SET(
		${_package}_FILES_
			${${_package}_SOURCES}
			${${_package}_MOC_SOURCES}
			${${_package}_UI_HEADERS}
			${${_package}_RESOURCE_SOURCES}
	)

	IF(WIN32)
		ADD_EXECUTABLE(${_name} WIN32 ${${_package}_FILES_})
		TARGET_LINK_LIBRARIES(${_name} ${QT_QTMAIN_LIBRARY})
	ELSE(WIN32)
		ADD_EXECUTABLE(${_name} ${${_package}_FILES_})
	ENDIF(WIN32)
	TARGET_LINK_LIBRARIES(${_name} ${QT_LIBRARIES})
	TARGET_LINK_LIBRARIES(${_name} ${QT_QTOPENGL_LIBRARY})

	INSTALL(TARGETS ${_name}
		RUNTIME DESTINATION ${SC3_PACKAGE_BIN_DIR}
		ARCHIVE DESTINATION ${SC3_PACKAGE_LIB_DIR}
		LIBRARY DESTINATION ${SC3_PACKAGE_LIB_DIR}
	)

	IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config/${_name}.cfg)
		INSTALL(
			FILES config/${_name}.cfg
			DESTINATION ${SC3_PACKAGE_CONFIG_DIR}
		)
	ENDIF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config/${_name}.cfg)
ENDMACRO(SC_ADD_GUI_EXECUTABLE)


MACRO(SC_ADD_GUI_TEST_EXECUTABLE _package _name)
	ADD_DEFINITIONS(-DSEISCOMP_TEST_DATA_DIR=\\\""${PROJECT_TEST_DATA_DIR}"\\\")
	INCLUDE(${QT_USE_FILE})

	# Create MOC Files
	IF (${_package}_MOC_HEADERS)
		QT4_WRAP_CPP(${_package}_MOC_SOURCES ${${_package}_MOC_HEADERS})
	ENDIF (${_package}_MOC_HEADERS)

	# Create UI Headers
	IF (${_package}_UI)
		QT4_WRAP_UI(${_package}_UI_HEADERS ${${_package}_UI})
		INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
		INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
	ENDIF (${_package}_UI)

	# Add resources
	IF (${_package}_RESOURCES)
		QT4_ADD_RESOURCES(${_package}_RESOURCE_SOURCES ${${_package}_RESOURCES})
	ENDIF (${_package}_RESOURCES)

	SET(
		${_package}_FILES_
			${${_package}_SOURCES}
			${${_package}_MOC_SOURCES}
			${${_package}_UI_HEADERS}
			${${_package}_RESOURCE_SOURCES}
	)

	IF(WIN32)
		ADD_EXECUTABLE(${_name} WIN32 ${${_package}_FILES_})
		TARGET_LINK_LIBRARIES(${_name} ${QT_QTMAIN_LIBRARY})
	ELSE(WIN32)
		ADD_EXECUTABLE(${_name} ${${_package}_FILES_})
	ENDIF(WIN32)
	TARGET_LINK_LIBRARIES(${_name} ${QT_LIBRARIES})
	TARGET_LINK_LIBRARIES(${_name} ${QT_QTOPENGL_LIBRARY})
ENDMACRO(SC_ADD_GUI_TEST_EXECUTABLE)


MACRO(SC_LINK_LIBRARIES _name)
	TARGET_LINK_LIBRARIES(${_name} ${ARGN})
ENDMACRO(SC_LINK_LIBRARIES)


MACRO(SC_LINK_LIBRARIES_INTERNAL _name)
	FOREACH(_lib ${ARGN})
		TARGET_LINK_LIBRARIES(${_name} seiscomp3_${_lib})
	ENDFOREACH(_lib)
ENDMACRO(SC_LINK_LIBRARIES_INTERNAL)


MACRO(SC_LIB_LINK_LIBRARIES _library_name)
	TARGET_LINK_LIBRARIES(seiscomp3_${_library_name} ${ARGN})
ENDMACRO(SC_LIB_LINK_LIBRARIES)


MACRO(SC_LIB_LINK_LIBRARIES_INTERNAL _library_name)
	FOREACH(_lib ${ARGN})
		TARGET_LINK_LIBRARIES(seiscomp3_${_library_name} seiscomp3_${_lib})
	ENDFOREACH(_lib)
ENDMACRO(SC_LIB_LINK_LIBRARIES_INTERNAL)


MACRO(SC_SWIG_LINK_LIBRARIES_INTERNAL _module)
    FOREACH(_lib ${ARGN})
	SWIG_LINK_LIBRARIES(${_module} seiscomp3_${_lib})
    ENDFOREACH(_lib)
ENDMACRO(SC_SWIG_LINK_LIBRARIES_INTERNAL)


MACRO(SC_SWIG_GET_MODULE_PATH _out)
    FILE(RELATIVE_PATH ${_out} ${SC3_PACKAGE_SOURCE_DIR}/swig ${CMAKE_CURRENT_SOURCE_DIR})
    SET(${_out} ${SC3_PACKAGE_LIB_DIR}${PYTHON_LIBRARY_SUFFIX}/${${_out}})
ENDMACRO(SC_SWIG_GET_MODULE_PATH)


MACRO(SC_LIB_VERSION _library_name _version _soversion)
    SET_TARGET_PROPERTIES(seiscomp3_${_library_name} PROPERTIES VERSION ${_version} SOVERSION ${_soversion})
ENDMACRO(SC_LIB_VERSION)


MACRO(SC_LIB_INSTALL_HEADERS ...)
	IF(${ARGC} GREATER 1)
		SET(_package_dir "${ARGV1}")
	ELSE(${ARGC} GREATER 1)
		FILE(RELATIVE_PATH _package_dir ${SC3_PACKAGE_SOURCE_DIR}/libs ${CMAKE_CURRENT_SOURCE_DIR})
	ENDIF(${ARGC} GREATER 1)

	INSTALL(FILES ${${ARGV0}_HEADERS}
		DESTINATION ${SC3_PACKAGE_INCLUDE_DIR}/${_package_dir}
	)

	IF (${ARGV0}_MOC_HEADERS)
		INSTALL(FILES ${${ARGV0}_MOC_HEADERS}
			DESTINATION ${SC3_PACKAGE_INCLUDE_DIR}/${_package_dir}
		)
	ENDIF (${ARGV0}_MOC_HEADERS)

	IF (${ARGV0}_UI_HEADERS)
		INSTALL(FILES ${${ARGV0}_UI_HEADERS}
			DESTINATION ${SC3_PACKAGE_INCLUDE_DIR}/${_package_dir}
		)
	ENDIF (${ARGV0}_UI_HEADERS)
ENDMACRO(SC_LIB_INSTALL_HEADERS)


MACRO(SC_PLUGIN_INSTALL _library_name _plugin_app)
	INSTALL(TARGETS seiscomp3_${_library_name}
		DESTINATION ${SC3_PACKAGE_SHARE_DIR}/plugins/${_plugin_app}
	)
ENDMACRO(SC_PLUGIN_INSTALL)


MACRO(SC_RAW_PLUGIN_INSTALL _library_name _plugin_app)
	INSTALL(TARGETS ${_library_name}
		DESTINATION ${SC3_PACKAGE_SHARE_DIR}/plugins/${_plugin_app}
	)
ENDMACRO(SC_RAW_PLUGIN_INSTALL)


MACRO(SC_INSTALL_DATA _package _path)
	INSTALL(FILES ${${_package}_DATA}
		DESTINATION ${SC3_PACKAGE_SHARE_DIR}/${_path}
	)
ENDMACRO(SC_INSTALL_DATA)


MACRO(SC_INSTALL_CONFIG _package)
	INSTALL(
		FILES ${${_package}_CONFIG}
		DESTINATION ${SC3_PACKAGE_CONFIG_DIR}
	)
ENDMACRO(SC_INSTALL_CONFIG)
