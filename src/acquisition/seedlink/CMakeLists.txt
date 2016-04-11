IF (COMPONENT_ENVIRONMENT)
	SET(env_dir env)
ENDIF (COMPONENT_ENVIRONMENT)

SUBDIRS(libs apps plugins ${env_dir})

INSTALL(
	DIRECTORY
		key
		templates
	DESTINATION
		acquisition
	PATTERN ".svn" EXCLUDE
)
