SET(SCPLOT_TARGET scplot)

SET(
    SCPLOT_HEADERS
    plotter.h
    gnuplot_i.h
)

SET(
    SCPLOT_SOURCES
    gnuplot_i.c
    plotter.cpp
	scplot.cpp
)

#INCLUDE_DIRECTORIES(/data/home/st/thoms/tmp/gnuplot_i-2.10/src)
#LINK_DIRECTORIES(/data/home/st/thoms/tmp/gnuplot_i-2.10/src)

SC_ADD_EXECUTABLE(SCPLOT ${SCPLOT_TARGET})
#TARGET_LINK_LIBRARIES(${SCPLOT_TARGET} ${Boost_program_options_LIBRARIES} gnuplot_i)
TARGET_LINK_LIBRARIES(${SCPLOT_TARGET} ${Boost_program_options_LIBRARIES} )
SC_LINK_LIBRARIES_INTERNAL(${SCPLOT_TARGET} log core)