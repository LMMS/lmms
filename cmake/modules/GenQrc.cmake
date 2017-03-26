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
	QT_ADD_RESOURCES(${RCCOUT} ${QRC_FILE})
ENDMACRO()


MACRO(QT_ADD_RESOURCES)
	IF(QT5)
		QT5_ADD_RESOURCES(${ARGN})
	ELSE()
		IF(${CMAKE_VERSION} VERSION_LESS 2.8.9)
			QT4_ADD_RESOURCES_2(${ARGN})
		ELSE()
			QT4_ADD_RESOURCES(${ARGN})
		ENDIF()
	ENDIF()
ENDMACRO()


# This macro is taken from CMake v2.8.9 so we can support CMake versions older
# than that.
#
# Copyright 2005-2009 Kitware, Inc.
# Distributed under the OSI-approved BSD License (the "License");
MACRO (QT4_ADD_RESOURCES_2 outfiles )
	QT4_EXTRACT_OPTIONS(rcc_files rcc_options ${ARGN})

	FOREACH (it ${rcc_files})
		GET_FILENAME_COMPONENT(outfilename ${it} NAME_WE)
		GET_FILENAME_COMPONENT(infile ${it} ABSOLUTE)
		GET_FILENAME_COMPONENT(rc_path ${infile} PATH)
		SET(outfile ${CMAKE_CURRENT_BINARY_DIR}/qrc_${outfilename}.cxx)

		SET(_RC_DEPENDS)
		IF(EXISTS "${infile}")
			#  parse file for dependencies
			#  all files are absolute paths or relative to the location of the qrc file
			FILE(READ "${infile}" _RC_FILE_CONTENTS)
			STRING(REGEX MATCHALL "<file[^<]+" _RC_FILES "${_RC_FILE_CONTENTS}")
			FOREACH(_RC_FILE ${_RC_FILES})
				STRING(REGEX REPLACE "^<file[^>]*>" "" _RC_FILE "${_RC_FILE}")
				IF(NOT IS_ABSOLUTE "${_RC_FILE}")
					SET(_RC_FILE "${rc_path}/${_RC_FILE}")
				ENDIF(NOT IS_ABSOLUTE "${_RC_FILE}")
				SET(_RC_DEPENDS ${_RC_DEPENDS} "${_RC_FILE}")
			ENDFOREACH(_RC_FILE)
			# Since this cmake macro is doing the dependency scanning for these files,
			# let's make a configured file and add it as a dependency so cmake is run
			# again when dependencies need to be recomputed.
			QT4_MAKE_OUTPUT_FILE("${infile}" "" "qrc.depends" out_depends)
			CONFIGURE_FILE("${infile}" "${out_depends}" COPY_ONLY)
		ELSE(EXISTS "${infile}")
			# The .qrc file does not exist (yet). Let's add a dependency and hope
			# that it will be generated later
			SET(out_depends)
		ENDIF(EXISTS "${infile}")

		ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
		COMMAND ${QT_RCC_EXECUTABLE}
		ARGS ${rcc_options} -name ${outfilename} -o ${outfile} ${infile}
		MAIN_DEPENDENCY ${infile}
		DEPENDS ${_RC_DEPENDS} "${out_depends}" VERBATIM)
		SET(${outfiles} ${${outfiles}} ${outfile})
	ENDFOREACH (it)

ENDMACRO (QT4_ADD_RESOURCES_2)
