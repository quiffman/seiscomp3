SUBDIRS(amplitudes magnitudes picker secondarypicker operator)

SET(
	CORE_PROCESSING_SOURCES
		parameters.cpp
		response.cpp
		sensor.cpp
		stream.cpp
		processor.cpp
		waveformprocessor.cpp
		timewindowprocessor.cpp
		application.cpp
		streambuffer.cpp
		waveformoperator.cpp

		detector.cpp
		picker.cpp
		secondarypicker.cpp
		amplitudeprocessor.cpp
		magnitudeprocessor.cpp

		amplitudes/MLv.cpp
		amplitudes/ML.cpp
		amplitudes/mb.cpp
		amplitudes/m_B.cpp
		amplitudes/mBc.cpp
		amplitudes/mBc_measure.cpp
		amplitudes/Mwp.cpp
		amplitudes/mwp_utils.c
		amplitudes/msbb.cpp
		amplitudes/Mjma.cpp

		magnitudes/MLv.cpp
		magnitudes/ML.cpp
		magnitudes/mb.cpp
		magnitudes/m_B.cpp
		magnitudes/mBc.cpp
		magnitudes/Mwp.cpp
		magnitudes/msbb.cpp
		magnitudes/Mjma.cpp

		picker/gfz.cpp
		picker/araic.cpp
		picker/bk.cpp
		picker/bk_ppick.cpp

		secondarypicker/S_l2.cpp

		operator/pipe.cpp
)

SET(
	CORE_PROCESSING_HEADERS
		parameters.h
		response.h
		sensor.h
		stream.h
		processor.h
		waveformprocessor.h
		timewindowprocessor.h
		application.h
		streambuffer.h
		waveformoperator.h

		detector.h
		picker.h
		secondarypicker.h
		amplitudeprocessor.h
		magnitudeprocessor.h
)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

SC_ADD_LIBRARY(CORE_PROCESSING processing)
SC_LIB_LINK_LIBRARIES(processing ${Boost_signals_LIBRARIES})
SC_LIB_LINK_LIBRARIES_INTERNAL(processing client seismology)
SC_LIB_INSTALL_HEADERS(CORE_PROCESSING)
