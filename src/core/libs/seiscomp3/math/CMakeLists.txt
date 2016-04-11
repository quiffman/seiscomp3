SUBDIRS(filter restitution)

SET(
	CORE_MATH_SOURCES
		math.cpp
		geo.cpp
		mean.cpp
		util.cpp
		coord.cpp
		polygon.cpp
		vector3.cpp
		matrix3.cpp
		tensor.cpp
		conversions.cpp
		filter.cpp
		fft.cpp
		filter/abs.cpp
		filter/const.cpp
		filter/cutoff.cpp
		filter/biquad.cpp
		filter/butterworth.cpp
		filter/iirfilter.cpp
		filter/iirdifferentiate.cpp
		filter/iirintegrate.cpp
		filter/rca.cpp
		filter/stalta.cpp
		filter/taper.cpp
		filter/rmhp.cpp
		filter/chainfilter.cpp
		filter/seismometers.cpp
		restitution/fft.cpp
		restitution/td.cpp
		restitution/transferfunction.cpp
)

SET(
	CORE_MATH_HEADERS
		decomp.ipp
		math.h
		geo.h
		hilbert.ipp
		minmax.ipp
		misc.ipp
		mean.h
		coord.h
		polygon.h
		vector3.h
		vector3.ipp
		matrix3.h
		matrix3.ipp
		tensor.h
		conversions.h
		conversions.ipp
		filter.h
		fft.h
)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR} ${FFTW3_INCLUDE_DIR})
ADD_DEFINITIONS(${FFTW3_DEFINITIONS})

SC_ADD_LIBRARY(CORE_MATH math)
#SC_LIB_LINK_LIBRARIES(math fftw3)
SC_LIB_LINK_LIBRARIES(math ${Boost_regex_LIBRARIES})
SC_LIB_LINK_LIBRARIES_INTERNAL(math core log)
SC_LIB_INSTALL_HEADERS(CORE_MATH)
