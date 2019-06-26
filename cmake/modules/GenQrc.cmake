# GenQrc.cmake - Copyright (c) 2015 Lukas W <lukaswhl/at/gmail.com>

# Generates a simple qrc file containing the given resource files ${ARGN}:
#     GEN_QRC(resources.qrc artwork.png icon.png PREFIX /icons)
# Files may also be added using a pattern with the GLOB keyword, e.g.:
#     GEN_QRC(resources.qrc GLOB *.png)
FUNCTION(GEN_QRC OUT_FILE)
	CMAKE_PARSE_ARGUMENTS(RC "" "PREFIX;GLOB" "" ${ARGN})

	IF(DEFINED RC_GLOB)
		FILE(GLOB GLOB_FILES ${RC_GLOB})
	ENDIF()

	# Set the standard prefix to "/" if none is given
	IF(NOT DEFINED RC_PREFIX)
		SET(RC_PREFIX "/")
	ENDIF()

	# We need to convert our list to a string in order to pass it to the script
	# on the command line.
	STRING(REPLACE ";" "\;" FILES "${RC_UNPARSED_ARGUMENTS};${GLOB_FILES}")

	SET(GENQRC_SCRIPT "${CMAKE_SOURCE_DIR}/cmake/scripts/GenQrc.cmake")
	ADD_CUSTOM_COMMAND(
		OUTPUT ${OUT_FILE}
		COMMAND ${CMAKE_COMMAND} -D OUT_FILE=${OUT_FILE} -D RC_PREFIX=${RC_PREFIX} -D FILES:list=${FILES} -D DIR=${CMAKE_CURRENT_SOURCE_DIR} -P "${GENQRC_SCRIPT}"
		DEPENDS ${GENQRC_SCRIPT}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		VERBATIM
	)
ENDFUNCTION()

# Generates a qrc file named ${QRC_OUT} from ${ARGN}, rccs it and returns Qt's
# output file.
# Must only be run once per CMakeLists.txt.
# Usage example:
#     ADD_GEN_QRC(RCC_OUTPUT resources.qrc icon.png manual.pdf)
#     ADD_EXECUTABLE(myexe main.cpp ${RCC_OUTPUT})
MACRO(ADD_GEN_QRC RCCOUT QRC_OUT)
	IF(NOT IS_ABSOLUTE ${QRC_OUT})
		SET(QRC_FILE "${CMAKE_CURRENT_BINARY_DIR}/${QRC_OUT}")
	ELSE()
		SET(QRC_FILE ${QRC_OUT})
	ENDIF()

	GEN_QRC(${QRC_FILE} "${ARGN}")
	QT5_ADD_RESOURCES(${RCCOUT} ${QRC_FILE})
ENDMACRO()
