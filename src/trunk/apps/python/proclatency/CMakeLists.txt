SET(PROCLATENCY_TARGET scproclat)

# The first file given is taken as the main
# script to execute
SET(
	PROCLATENCY_FILES
		scproclat.py
)

SC_ADD_PYTHON_EXECUTABLE(PROCLATENCY)
