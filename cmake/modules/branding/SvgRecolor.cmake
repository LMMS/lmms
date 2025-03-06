# Recolor an SVG using search/replace techniques
#
# Copyright (c) 2025, Tres Finocchiaro, <tres.finocchiaro@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

find_program(SvgRecolor_FOUND true)

function(svg_recolor search_list replace_list source_file result)
	include(SetupBrandingEnv)
	setup_env()
	file(READ "${source_file}" svg_data_orig)
	set(svg_data "${svg_data_orig}")
	# set(var "${${var}}") will read and expand a quoted list by name
	set(search_list "${${search_list}}")
	set(replace_list "${${replace_list}}")
	list(LENGTH search_list length)
	math(EXPR length "${length}-1")
	foreach(i RANGE ${length})
		list(GET search_list ${i} search)
		list(GET replace_list ${i} replace)
		if(BRANDING_DEBUG)
			message(" svg_recolor ${i}: ${search} --> ${replace}")
		endif()
		string(REPLACE "${search}" "${replace}" svg_data "${svg_data}")
	endforeach()

	get_filename_component(source_file_name "${source_file}" NAME)
	set(destination_file "${CPACK_BRANDED_DIR}/${source_file_name}")
	if("${svg_data}" STREQUAL "${svg_data_orig}")
		message(FATAL_ERROR "Source and destination files are identical, recolor failed\n"
		"Set BRANDING_DEBUG=ON to troubleshoot")
	endif()
	file(WRITE "${destination_file}" "${svg_data}")
	if(BRANDING_DEBUG)
		message(STATUS " svg_recolor: ${source_file} --> ${destination_file}")
	endif()
	set(${result} "${destination_file}" PARENT_SCOPE)
endfunction()