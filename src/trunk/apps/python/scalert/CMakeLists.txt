SET(SCALERT_TARGET scalert)

SET(
	SCALERT_FILES
		scalert.py
)

SET(
	SCALERT_DATA
		data/alert.sh
		data/event.sh
)


SC_ADD_PYTHON_EXECUTABLE(SCALERT)
SC_INSTALL_DATA(SCALERT scalert)
