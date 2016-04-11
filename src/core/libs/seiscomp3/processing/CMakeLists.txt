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
		mBc.cpp
		mwp.c

		detector.cpp
		picker.cpp
		gfzpicker.cpp
		araicpicker.cpp
		bkpicker.cpp
		picker_ppick.cpp
		amplitudeprocessor_MLv.cpp
		amplitudeprocessor_ML.cpp
		amplitudeprocessor_mb.cpp
		amplitudeprocessor_m_B.cpp
		amplitudeprocessor_mBc.cpp
 		amplitudeprocessor_Mwp.cpp
		amplitudeprocessor_msbb.cpp
		amplitudeprocessor_Mjma.cpp
		amplitudeprocessor.cpp
		magnitudeprocessor_MLv.cpp
		magnitudeprocessor_ML.cpp
		magnitudeprocessor_mb.cpp
		magnitudeprocessor_m_B.cpp
		magnitudeprocessor_mBc.cpp
		magnitudeprocessor_Mwp.cpp
		magnitudeprocessor_msbb.cpp
		magnitudeprocessor_Mjma.cpp
		magnitudeprocessor.cpp
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

		detector.h
		picker.h
		gfzpicker.h
		araicpicker.h
		bkpicker.h
		amplitudeprocessor.h
		amplitudeprocessors.h
		amplitudeprocessor_MLv.h
		amplitudeprocessor_ML.h
		amplitudeprocessor_mb.h
		amplitudeprocessor_m_B.h
		amplitudeprocessor_Mwp.h
		amplitudeprocessor_msbb.h
		amplitudeprocessor_Mjma.h
		magnitudeprocessors.h
		magnitudeprocessor.h
		magnitudeprocessor_MLv.h
		magnitudeprocessor_ML.h
		magnitudeprocessor_mb.h
		magnitudeprocessor_m_B.h
		magnitudeprocessor_Mwp.h
		magnitudeprocessor_msbb.h
		magnitudeprocessor_Mjma.h
)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

SC_ADD_LIBRARY(CORE_PROCESSING processing)
SC_LIB_LINK_LIBRARIES(processing ${Boost_signals_LIBRARIES})
SC_LIB_LINK_LIBRARIES_INTERNAL(processing client seismology)
SC_LIB_INSTALL_HEADERS(CORE_PROCESSING)
