FIND_PACKAGE(Boost REQUIRED program_options thread iostreams filesystem regex signals)
# Boost 1.46.0 and above use Filesystem v3 by default.
ADD_DEFINITIONS("-DBOOST_FILESYSTEM_VERSION=2")

IF (WIN32)
	ADD_DEFINITIONS("-DBOOST_ALL_NO_LIB")
	ADD_DEFINITIONS("-DBOOST_LIB_DIAGNOSTIC")
ENDIF (WIN32)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
OPTION(COMPONENT_ENVIRONMENT "Install environment files (setup, seiscomp config, ...)" ON)

FILE(GLOB files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "[^.]*")
SET(dirs "")
FOREACH(dir ${files})
	IF (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${dir})
		SET(dirs ${dirs} ${dir})
		MESSAGE(STATUS "Adding subdir ${dir}")
	ENDIF (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${dir})
ENDFOREACH(dir ${files})

SUBDIRS(${dirs})
