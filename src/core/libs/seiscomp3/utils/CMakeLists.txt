SET(
	CORE_UTILS_SOURCES
		timer.cpp
		base64.cpp
		datetime.cpp
		replace.cpp
		files.cpp
		leparser.cpp
		misc.cpp
)

SET(
	CORE_UTILS_HEADERS
		base64.h
		base64.ipp 
		timer.h
		datetime.h
		replace.h
		files.h
		leparser.h
		misc.h
)


SC_ADD_LIBRARY(CORE_UTILS utils)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
SC_LIB_LINK_LIBRARIES(utils ${Boost_thread_LIBRARIES})
SC_LIB_LINK_LIBRARIES_INTERNAL(utils core)
SC_LIB_INSTALL_HEADERS(CORE_UTILS)
