SET(BULLETIN_TARGET scbulletin)

# The first file given is taken as the main
# script to execute
SET(
	BULLETIN_FILES
		scbulletin.py
)

SC_ADD_PYTHON_EXECUTABLE(BULLETIN)
