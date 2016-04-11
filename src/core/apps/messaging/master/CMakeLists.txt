SUBDIRS(plugins)

SET(
    MASTER_SOURCES
	clientdb.cpp
	master.cpp
	scmaster.cpp
)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(.)

SC_ADD_EXECUTABLE(MASTER scmaster)
TARGET_LINK_LIBRARIES(scmaster ${Boost_program_options_LIBRARIES})
SC_LINK_LIBRARIES_INTERNAL(scmaster client core utils)
