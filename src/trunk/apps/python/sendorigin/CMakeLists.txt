SET(SENDORIGIN_TARGET scsendorigin)

# The first file given is taken as the main
# script to execute
SET(
	SENDORIGIN_FILES
		scsendorigin.py
)

SC_ADD_PYTHON_EXECUTABLE(SENDORIGIN)
