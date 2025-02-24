# GenQrc.cmake - Copyright (c) 2015 Lukas W <lukaswhl/at/gmail.com>

# Generates a simple qrc file named ${QRC_NAME} containing the given resource
# files ${ARGN}, rccs it, and returns Qt's output file.
# Must only be run once per CMakeLists.txt.
# Usage example:
#     add_gen_qrc(RCC_OUTPUT resources.qrc artwork.png icon.png PREFIX /icons)
#     add_executable(myexe main.cpp ${RCC_OUTPUT})
# Files may also be added using a pattern with the GLOB keyword, e.g.:
#     add_gen_qrc(RCC_OUTPUT resources.qrc GLOB *.png)
function(add_gen_qrc RCC_OUT QRC_NAME)
	cmake_parse_arguments(RC "" "PREFIX;GLOB" "" ${ARGN})

	# Get the absolute paths for the generated files
	if(IS_ABSOLUTE "${QRC_NAME}")
		set(QRC_FILE "${QRC_NAME}")
	else()
		set(QRC_FILE "${CMAKE_CURRENT_BINARY_DIR}/${QRC_NAME}")
	endif()
	get_filename_component(RESOURCE_NAME "${QRC_FILE}" NAME_WE)
	get_filename_component(OUTPUT_DIR "${QRC_FILE}" DIRECTORY)
	set(CPP_FILE "${OUTPUT_DIR}/qrc_${RESOURCE_NAME}.cpp")

	# Set the standard prefix to "/" if none is given
	if(NOT DEFINED RC_PREFIX)
		set(RC_PREFIX "/")
	endif()

	# Determine input files
	set(FILES ${RC_UNPARSED_ARGUMENTS})
	if(DEFINED RC_GLOB)
		file(GLOB GLOB_FILES "${RC_GLOB}")
		list(APPEND FILES ${GLOB_FILES})
	endif()

	# Add the command to generate the QRC file
	set(GENQRC_SCRIPT "${CMAKE_SOURCE_DIR}/cmake/scripts/GenQrc.cmake")
	add_custom_command(
		OUTPUT "${QRC_FILE}"
		COMMAND "${CMAKE_COMMAND}"
			-D "OUT_FILE=${QRC_FILE}"
			-D "RC_PREFIX=${RC_PREFIX}"
			-D "FILES:list=${FILES}"
			-D "DIR=${CMAKE_CURRENT_SOURCE_DIR}"
			-P "${GENQRC_SCRIPT}"
		DEPENDS "${GENQRC_SCRIPT}"
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		VERBATIM
	)

	# Add the command to compile the QRC file
	# Note: we can't use `qt5_add_resources` or `AUTORCC` here; we have to add
	# the command ourselves instead. This is in order to handle dependencies
	# correctly: the QRC file is generated at build time, so the dependencies
	# of the compiled file can't be automatically determined at configure time.
	# Additionally, `qt5_add_resources` adds unnecessary dependencies for
	# generated QRC files, which can cause dependency cycles with some
	# generators. See issue #6177.
	add_custom_command(
		OUTPUT "${CPP_FILE}"
		COMMAND Qt${QT_VERSION_MAJOR}::rcc
			--name "${RESOURCE_NAME}"
			--output "${CPP_FILE}"
			"${QRC_FILE}"
		DEPENDS "${QRC_FILE}" ${FILES}
		VERBATIM
	)

	# Flag the generated files to be ignored by automatic tool processing
	set_source_files_properties("${QRC_FILE}" PROPERTIES
		SKIP_AUTORCC ON # We added the rcc command for this manually
	)
	set_source_files_properties("${CPP_FILE}" PROPERTIES
		SKIP_AUTOMOC ON # The rcc output file has no need for moc or uic
		SKIP_AUTOUIC ON
	)

	# Return the rcc output file
	set("${RCC_OUT}" "${CPP_FILE}" PARENT_SCOPE)
endfunction()
