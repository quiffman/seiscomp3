INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})
INCLUDE_DIRECTORIES(../../libs/3rd-party/libslink)
INCLUDE_DIRECTORIES(../../libs/3rd-party/qlib2)

ADD_EXECUTABLE(timeout timeout.c)
#ADD_EXECUTABLE(mseed_last_time mseed_last_time.c)
ADD_EXECUTABLE(trylock trylock.c)
ADD_EXECUTABLE(run_with_lock run_with_lock.c lockutil.c)
ADD_EXECUTABLE(load_timetable load_timetable.c)
ADD_EXECUTABLE(waitlock waitlock.c)

TARGET_LINK_LIBRARIES(load_timetable slink qlib2 ${LIBXML2_LIBRARIES})

INSTALL(
	TARGETS
		timeout
		trylock
		waitlock
		run_with_lock
	RUNTIME DESTINATION bin
)

INSTALL(
	TARGETS
		load_timetable
	RUNTIME DESTINATION acquisition/bin
)
