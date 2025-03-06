# Convert an SVG to png at the sizes specified by size_list
#
# Copyright (c) 2025, Tres Finocchiaro, <tres.finocchiaro@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

cmake_policy(SET CMP0053 NEW)

function(svg_convert size_list source_file output_pattern)
	include(SetupBrandingEnv)
	setup_env()

	find_svg_tool(svg_tool)
	if(NOT svg_tool)
		message(FATAL_ERROR "Could not find a suitable svg conversion tool")
	endif()
	foreach(size ${size_list})
		if(NOT EXISTS "${source_file}")
			message(FATAL_ERROR "SVG file does not exist: ${source_file}")
		endif()

		# Will call function called "${svg_tool}_convert"
		# e.g. "rsvg_convert", "inkscape_convert", etc
		# Each function must accept the following parameters:
		# - source_file:        e.g. "/path/to/lmms.svg"
		# - size:               e.g. "16" or "16@2"
		# - WORKING_DIRECTORY:  e.g. "${CMAKE_BINARY_DIR}"
		# - pattern:            e.g. /path/to/lmms-"%size%x%size%@%mult%.png
		# - command_echo:       e.g. "NONE" or "STDOUT"
		# - result:             e.g. "MY_VAR": Expanded "${MY_VAR}" will contain the path to generated file
		#
		# Each function MUST throw FATAL_ERROR if unsuccessful
		cmake_language(CALL ${svg_tool}_convert
			"${source_file}"
			${size}
			"${CPACK_BRANDED_DIR}"
			"${output_pattern}"
			${COMMAND_ECHO}
			png_rendered
		)
		if(BRANDING_DEBUG)
			message(STATUS " ${svg_tool}_convert: ${source_file} --> ${png_rendered}")
		endif()
	endforeach()
endfunction()

# rsvg-convert is the preferred tool
function(rsvg_convert source_file size working_dir pattern command_echo result)
	patternize_file("${source_file}" ${size} "${pattern}" scaled_size png_out)

	file(REMOVE "${png_out}")
	execute_process(COMMAND rsvg-convert
		"${source_file}"
		-w "${scaled_size}"
		-o "${png_out}"
		WORKING_DIRECTORY "${working_dir}"
		COMMAND_ECHO ${command_echo}
		COMMAND_ERROR_IS_FATAL ANY
	)

	set(${result} "${png_out}" PARENT_SCOPE)
endfunction()

# Inkscape is slower than rsvg-convert, but it works just fine
function(inkscape_convert source_file size working_dir pattern command_echo result)
	patternize_file("${source_file}" ${size} "${pattern}" scaled_size png_out)

	# Inkscape prefers units as 90-dpi
	file(REMOVE "${png_out}")
	execute_process(COMMAND inkscape "${source_file}"
		--batch-process
		--export-dpi=72
		--export-type=png
		--export-width=${scaled_size}
		"--export-filename=${png_out}"
		WORKING_DIRECTORY "${working_dir}"
		OUTPUT_QUIET
		ERROR_QUIET
		COMMAND_ECHO ${command_echo}
		COMMAND_ERROR_IS_FATAL ANY
		ERROR_VARIABLE inkscape_output
	)

	# Inkscape doesn't properly set error codes
	if(NOT EXISTS "${png_out}")
		# Blindly dump whatever was on stderr
		message(FATAL_ERROR " inkscape_convert: ${png_out} was not created:\n${inkscape_output}")
	endif()

	set(${result} "${png_out}" PARENT_SCOPE)
endfunction()

# Gimp is the slowest tool available
function(gimp_convert source_file size working_dir pattern command_echo result)
	patternize_file("${source_file}" ${size} "${pattern}" scaled_size png_out)
	# Use scheme language for gimp batch conversion

	file(READ "${source_directory}/cmake/modules/branding/gimp_convert.scm.in" gimp_lisp)
	string(REPLACE "@source_file@" "${source_file}" gimp_lisp "${gimp_lisp}")
	string(REPLACE "@width@" "${scaled_size}" gimp_lisp "${gimp_lisp}")
	string(REPLACE "@height@" "${scaled_size}" gimp_lisp "${gimp_lisp}")
	string(REPLACE "@resolution@" 72 gimp_lisp "${gimp_lisp}")
	string(REPLACE "@png_out@" "${png_out}" gimp_lisp "${gimp_lisp}")

	file(REMOVE "${png_out}")
	execute_process(COMMAND gimp
		--no-interface
		--console-messages
		--batch "${gimp_lisp}"
		--batch  "(gimp-quit 0)" # quit must be a separate batch line or it will hang
		WORKING_DIRECTORY "${working_dir}"
		OUTPUT_QUIET
        ERROR_QUIET
		COMMAND_ECHO ${command_echo}
		COMMAND_ERROR_IS_FATAL ANY
	)

	# Gimp doesn't properly set error codes
	if(NOT EXISTS "${png_out}")
		message(FATAL_ERROR " gimp_convert: ${png_out} was not created:\n...\n${gimp_lisp}\n...\n")
	endif()

	set(${result} "${png_out}" PARENT_SCOPE)
endfunction()

# Find an svg tool on the command line
#
# Sets the variable provided as "${return}" to the tool name.
# will shorten "rsvg-convert' to simply "rsvg" for dynamic function
# name calling.  All other tools should be their name and a corresponding
# "<foo>_convert" must be declared to handle the command-line processing.
#
# For most systems, simply calling "<package manager> install rsvg-convert"
# will provide the best tool for the job
#
# Note: "execute_process(...)" is preferred over "find_package(...)" for now
# due to unavailiblity of "FindGimp.cmake", etc.
function(find_svg_tool return)
	# use cached version
	if(SVG_CONVERT_TOOL_CACHE)
		set(${return} "${SVG_CONVERT_TOOL_CACHE}" PARENT_SCOPE)
		return()
	endif()

	# sorted by priority, rsvg-convert is fastest
	set(tools
		rsvg-convert
		inkscape
		gimp
	)

	foreach(tool ${tools})
		message(STATUS " svg-convert: Checking for ${tool}...")
		find_program(TOOL_FOUND "${tool}")
		# assume any output is good output
		if(TOOL_FOUND)
			message(STATUS " svg-convert: found \"${tool}\" using")
			# isolate "rsvg" if needed
			string(REPLACE "-convert" "" tool "${tool}")
			# cache it
			set(SVG_CONVERT_TOOL_CACHE "${tool}" CACHE STRING "SVG tool for image conversion")
			# return it
			set(${return} "${tool}" PARENT_SCOPE)
			return()
		endif()
	endforeach()
endfunction()

find_svg_tool(SvgConvert_FOUND)

# Patternize a filename so we can write it to well-known locations
#
# Example:
#   patternize_file(background.svg 64@2 "%name%@%mult%x.png" size file)
#
#   source_file:
#     background.svg, lmms.svg, etc
#
#   size:
#     32, 64, 64@2, etc
#
#   pattern:
# 	  background@2x.png = %name%@%mult%x.png
# 	  icons/16x16@2/apps/lmms.png = icons/%size%x%size%@%mult%.png
#
#   result_size, result_file: output variables
#
# There is no handling for escaping %'s
function(patternize_file source_file size pattern result_size result_file)
	# Calculate 'multiplier' 'unscaled_size' 'scaled_size' from provided size (e.g. '128' or '128@2')
	if(size MATCHES "@")
		string(REPLACE "@" ";" parts "${size}")
		list(GET parts 0 unscaled_size)
		list(GET parts 1 multiplier)
		math(EXPR scaled_size "${unscaled_size}*${multiplier}")
	else()
		set(unscaled_size "${size}")
		set(multiplier 1)
		set(scaled_size "${size}")
	endif()

	# Handle %mult%
	if(multiplier EQUAL 1)
		# Assume no one wants "@1" or "@2x" in a filename
		string(REPLACE "@%mult%x." "." result_file_name "${pattern}")
		string(REPLACE "@%mult%" "" result_file_name "${result_file_name}")
	else()
		# Special handling for apple's "@2x" notation
		string(REPLACE "@%mult%x." "@${multiplier}x." result_file_name "${pattern}")
		string(REPLACE "%mult%" "${multiplier}" result_file_name "${pattern}")
	endif()

	# Handle %size%
	string(REPLACE "%size%" "${unscaled_size}" result_file_name "${result_file_name}")

	# Handle %name%
	get_filename_component(name "${source_file}" NAME_WLE)
	string(REPLACE "%name%" "${name}" result_file_name "${result_file_name}")

	# Ensure parent directory exists
	get_filename_component(parent_dir "${result_file_name}" DIRECTORY)
	if(NOT EXISTS "${parent_dir}")
		file(MAKE_DIRECTORY "${parent_dir}")
	endif()

	set(${result_size} "${scaled_size}" PARENT_SCOPE)
	set(${result_file} "${result_file_name}" PARENT_SCOPE)
endfunction()