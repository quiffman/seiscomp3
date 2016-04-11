SET(WFPARAM_TARGET scwfparam)

SET(
	WFPARAM_SOURCES
		main.cpp
		app.cpp
		wfparam.cpp
		util.cpp
		msg.cpp
		processors/pgav.cpp
)

INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

SC_ADD_EXECUTABLE(WFPARAM ${WFPARAM_TARGET})
SC_LINK_LIBRARIES_INTERNAL(${WFPARAM_TARGET} processing datamodel_sm)
