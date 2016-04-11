SET(PROCALERT_TARGET sc2pa)

# The first file given is taken as the main
# script to execute
SET(
	PROCALERT_FILES
		sc2pa.py
)

SC_ADD_PYTHON_EXECUTABLE(PROCALERT)
