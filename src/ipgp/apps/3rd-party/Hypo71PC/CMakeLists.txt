IF(SC_IPGP_HYPO71)
	ENABLE_LANGUAGE(Fortran)

	SET(HYPO71_TARGET Hypo71PC)

	IF(Fortran_COMPILER_NAME STREQUAL "gfortran")
		SET(CMAKE_Fortran_FLAGS "-g -Bstatic")
	ENDIF(Fortran_COMPILER_NAME STREQUAL "gfortran")

	SET(
		HYPO71_SOURCES
			main.f
			hypo1m2.f
			hypo2.f
			hypo3.f
			hypo4.f
			single.f
			ytrv.f
			input1.f
			timz3.f
			geo_sp.f
	)

	INCLUDE_DIRECTORIES(.)

	SC_ADD_EXECUTABLE(HYPO71 ${HYPO71_TARGET})
ELSE(SC_IPGP_HYPO71)
	MESSAGE(STATUS "... Hypo71 is not built, disabled by config")
ENDIF(SC_IPGP_HYPO71)
