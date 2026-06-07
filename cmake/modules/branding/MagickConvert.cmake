# Convert an SVG to Apple icns file
#
# Copyright (c) 2025, Tres Finocchiaro, <tres.finocchiaro@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

find_program(MagickConvert_FOUND magick)

# Create NSIS-suitable-BMP3 top banner image by drawing the LMMS icon on a white canvas
function(nsis_banner source_file destination_file size)
	include(SetupBrandingEnv)
	setup_env()

	# Temporary file for drawing our banner to
	set(temp_file "temp_canvas.bmp")

	# Create a blank canvas
	get_filename_component(destination_dir "${destination_file}" DIRECTORY)
	file(MAKE_DIRECTORY "${destination_dir}")
	execute_process(COMMAND magick
		-size "${size}"
		canvas: "${temp_file}"
		WORKING_DIRECTORY "${CPACK_BRANDED_DIR}"
		OUTPUT_QUIET
		COMMAND_ECHO ${COMMAND_ECHO}
		COMMAND_ERROR_IS_FATAL ANY
	)
	# Draw the provided image to it
	get_filename_component(file_name "${destination_file}" NAME)
	execute_process(COMMAND magick
		"${CPACK_BRANDED_DIR}/${temp_file}"
		"${source_file}"
		-gravity center
		-geometry -25-1
		-composite
		-depth 8 -compress none
		"BMP3:${file_name}" # imagemagick format specifiers don't allow full paths
		WORKING_DIRECTORY "${destination_dir}"
		OUTPUT_QUIET
		COMMAND_ECHO ${COMMAND_ECHO}
		COMMAND_ERROR_IS_FATAL ANY
	)

	# Cleanup
	file(REMOVE "${temp_file}")
endfunction()

# Convert a png to a NSIS-suitable-BMP3
function(nsis_bmp source_file destination_file)
	include(SetupBrandingEnv)
	setup_env()

	# Convert image to BMP3
	get_filename_component(destination_dir "${destination_file}" DIRECTORY)
	get_filename_component(file_name "${destination_file}" NAME)
	execute_process(COMMAND magick
		"${source_file}"
		-depth 8 -compress none
		"BMP3:${file_name}" # imagemagick format specifiers don't allow full paths
		WORKING_DIRECTORY "${destination_dir}"
		OUTPUT_QUIET
		COMMAND_ECHO ${COMMAND_ECHO}
		COMMAND_ERROR_IS_FATAL ANY
	)
endfunction()

# Convert the source image to a multi-res Windows ".ico" file
function(ico_convert source_file destination_file)
	include(SvgConvert)
	include(SetupBrandingEnv)
	setup_env()

	# First, create icons at the standard resolutions
	get_filename_component(name "${source_file}" NAME_WLE)
	file(REMOVE_RECURSE "${CPACK_BRANDED_DIR}/${name}.iconset")

	set(sizes 16 20 24 30 32 36 40 48 60 64 72 80 96 128 256)
	svg_convert("${sizes}" "${source_file}" "${CPACK_BRANDED_DIR}/%name%.iconset/icon_%size%x%size%.png")

	# Collect all files for convert
	foreach(size ${sizes})
		list(APPEND files "${CPACK_BRANDED_DIR}/${name}.iconset/icon_${size}x${size}.png")
	endforeach()

	# Create the ico file
	get_filename_component(destination_dir "${destination_file}" DIRECTORY)
	file(MAKE_DIRECTORY "${destination_dir}")
	execute_process(COMMAND magick
		${files}
		"${destination_file}"
		WORKING_DIRECTORY "${CPACK_BRANDED_DIR}"
		OUTPUT_QUIET
		COMMAND_ECHO ${COMMAND_ECHO}
		COMMAND_ERROR_IS_FATAL ANY
	)

	file(REMOVE "${CPACK_BRANDED_DIR}/${name}.iconset")
	message(STATUS " ico_convert: ${source_file} --> ${destination_file}")
endfunction()