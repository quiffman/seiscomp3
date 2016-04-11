SET(SCREAMPLUGIN_SOURCES
	dispatch.c
	gcf.c
	gputil.c
	main.c
	map.c
	scream.c
	util.c
	../plugin.c
)

INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(..)

ADD_EXECUTABLE(scream_plugin ${SCREAMPLUGIN_SOURCES})
TARGET_LINK_LIBRARIES(scream_plugin m)

INSTALL(TARGETS scream_plugin RUNTIME DESTINATION acquisition/bin)
