# SET(
# 	ADMIN_HEADER
# 	admin.h
# )
# 
# SET(
#     ADMIN_SOURCES
# 	admin.cpp
# 	scadmin.cpp
# )
# 
# INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
# 
# SC_ADD_EXECUTABLE(ADMIN scadmin)
# TARGET_LINK_LIBRARIES(scadmin ${Boost_program_options_LIBRARIES})
# SC_LINK_LIBRARIES_INTERNAL(scadmin utils log)

SET(SCADMIN_TARGET scadmin)

SET(
    SCADMIN_HEADERS
    admin.h
)

SET(
    SCADMIN_SOURCES
    admin.cpp
    scadmin.cpp
)

SC_ADD_EXECUTABLE(SCADMIN ${SCADMIN_TARGET})
TARGET_LINK_LIBRARIES(${SCADMIN_TARGET} ${Boost_program_options_LIBRARIES} )
SC_LINK_LIBRARIES_INTERNAL(${SCADMIN_TARGET} log communication core utils)
