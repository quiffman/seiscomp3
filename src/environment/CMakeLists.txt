IF ( COMPONENT_ENVIRONMENT )
        IF ( ${PYTHON_VERSION} STREQUAL "2.3" )
                INSTALL(
                        DIRECTORY
                                .
                        DESTINATION
                                .
                        USE_SOURCE_PERMISSIONS
                        PATTERN ".svn" EXCLUDE
                        PATTERN "CMakeLists.txt" EXCLUDE
                        PATTERN "*~" EXCLUDE
            )
        ELSE ( ${PYTHON_VERSION} STREQUAL "2.3" )
                INSTALL(
                        DIRECTORY
                                .
                        DESTINATION
                                .
                        USE_SOURCE_PERMISSIONS
                        PATTERN ".svn" EXCLUDE
                        PATTERN "CMakeLists.txt*" EXCLUDE
                        PATTERN "*~" EXCLUDE
                        PATTERN "decimal.py" EXCLUDE
            )
        ENDIF ( ${PYTHON_VERSION} STREQUAL "2.3" )
ENDIF ( COMPONENT_ENVIRONMENT )
