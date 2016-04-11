SET(SCVOICE_TARGET scvoice)

SET(
	SCVOICE_FILES
		scvoice.py
)

SET(
	SCVOICE_DATA
		data/alert.sh
		data/event.sh
)


SC_ADD_PYTHON_EXECUTABLE(SCVOICE)
SC_INSTALL_DATA(SCVOICE scvoice)
