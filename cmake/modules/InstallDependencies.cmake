include(GetPrerequisites)
include(CMakeParseArguments)

CMAKE_POLICY(SET CMP0011 NEW)
CMAKE_POLICY(SET CMP0057 NEW)

function(make_absolute var)
	get_filename_component(abs "${${var}}" ABSOLUTE BASE_DIR "${CMAKE_INSTALL_PREFIX}")
	set(${var} ${abs} PARENT_SCOPE)
endfunction()

# Reads lines of a file into a list, skipping '#' comment lines
function(READ_LIST_FILE FILE VAR)
	file(STRINGS "${FILE}" list)

	set(result "")
	foreach(item ${list})
		string(STRIP "${item}" item)
		if(item STREQUAL "" OR item MATCHES "^\#")
			continue()
		endif()
		list(APPEND result "${item}")
	endforeach()

	set(${VAR} ${result} PARENT_SCOPE)
endfunction()

function(make_all_absolute list_var)
	set(result "")
	foreach(file ${${list_var}})
		make_absolute(file)
		list(APPEND result ${file})
	endforeach()
	set(${list_var} ${result} PARENT_SCOPE)
endfunction()

if(CMAKE_BINARY_DIR)
	set(tmp_lib_dir "${CMAKE_BINARY_DIR}/bundled-libraries")
elseif(CMAKE_HOST_UNIX)
	set(tmp_lib_dir "/tmp/bundled-libraries")
elseif(DEFINED ENV{TEMP})
	set(tmp_lib_dir "$ENV{TMP}/bundled-libraries")
else()
	message(FATAL_ERROR "Can't find a temp dir for libraries")
endif()

# Like file(INSTALL), but resolves symlinks
function(install_file_resolved file destination)

	get_filename_component(file_name "${file}" NAME)
	if(IS_SYMLINK "${file}")
		get_filename_component(real_path "${file}" REALPATH)
		get_filename_component(real_name "${real_path}" NAME)
		file(COPY "${real_path}" DESTINATION "${tmp_lib_dir}")
		file(RENAME "${tmp_lib_dir}/${real_name}" "${tmp_lib_dir}/${file_name}")
		set(file_path "${tmp_lib_dir}/${file_name}")
	else()
		set(file_path "${file}")
	endif()

	file(INSTALL "${file_path}" DESTINATION "${destination}")
endfunction()

function(install_resolved)
	cmake_parse_arguments("" "" "DESTINATION" "FILES" ${ARGN})
	foreach(file ${_FILES})
		install_file_resolved("${file}" "${_DESTINATION}")
	endforeach()
endfunction()

if(CMAKE_CROSSCOMPILING)
	# If we're cross-compiling, GetPrerequisites may not be able to find system libraries such as kernel32.dll because
	# they're supplied by the toolchain. To suppress thousands of lines of warnings being printed to the console, we
	# override gp_resolved_file_type to return "system" for any library in ${IGNORE_LIBS} without trying to resolve the
	# file first.
	# GetPrerequisites supports using an override function called gp_resolved_file_type_override, but it's not suited
	# for our purpose because it's only called by gp_resolved_file_type *after* trying to resolve the file.
	function(gp_resolved_file_type original_file file exepath dirs type_var)
		set(file_find "${file}")
		if(_IGNORE_CASE)
			# On case-insensitive systems, convert to upper characters to respect it
			string(TOUPPER "${file_find}" file_find)
		endif()
		SET(IGNORE_LIBS ${_IGNORE_LIBS} CACHE INTERNAL "Ignored library names" FORCE)
		if(IGNORE_LIBS AND ${file_find} IN_LIST IGNORE_LIBS)
			set(${type_var} system PARENT_SCOPE)
		else()
			#_gp_resolved_file_type(${ARGV})
			_gp_resolved_file_type("${original_file}" "${file}" "${exepath}" "${dirs}" "${type_var}" ${ARGN})
		endif()
	endfunction()
endif()

function(INSTALL_DEPENDENCIES)
	cmake_parse_arguments("" "INCLUDE_SYSTEM;IGNORE_CASE" "GP_TOOL;DESTINATION;IGNORE_LIBS_FILE" "FILES;LIB_DIRS;SEARCH_PATHS;IGNORE_LIBS" ${ARGN})

	# Make paths absolute
	make_absolute(_DESTINATION)
	make_all_absolute(_FILES)
	make_all_absolute(_LIB_DIRS)
	make_all_absolute(_SEARCH_PATHS)

	if(_INCLUDE_SYSTEM)
		set(EXCLUDE_SYSTEM 0)
	else()
		set(EXCLUDE_SYSTEM 1)
	endif()

	if(_IGNORE_LIBS_FILE)
		READ_LIST_FILE("${_IGNORE_LIBS_FILE}" _IGNORE_LIBS)
		if(_IGNORE_CASE)
			# On case-insensitive systems, convert to upper characters to respect it
			string(TOUPPER "${_IGNORE_LIBS}" _IGNORE_LIBS)
		endif()
		SET(IGNORE_LIBS ${_IGNORE_LIBS} CACHE INTERNAL "Ignored library names" FORCE)
	endif()

	if(_GP_TOOL)
		set(gp_tool "${_GP_TOOL}")
	endif()

	set(prereqs "")
	foreach(file ${_FILES})
		get_filename_component(file_name "${file}" NAME)
		message("-- Finding prerequisites of ${file_name}")
		find_prerequisites("${file}" _prereqs
			${EXCLUDE_SYSTEM} # exclude system files
			1 # recurse
			""
			"${_LIB_DIRS}"
			"${_SEARCH_PATHS}"
			"${_IGNORE_LIBS}"
		)

		list(APPEND prereqs ${_prereqs})
	endforeach()

	list(REMOVE_DUPLICATES prereqs)

	foreach(prereq ${prereqs})
		get_filename_component(prereq_name "${prereq}" NAME)

		foreach(rpath ${_SEARCH_PATHS})
			if(EXISTS "${rpath}/${prereq_name}")
				list(REMOVE_ITEM prereqs "${prereq}")
				break()
			endif()
		endforeach()
	endforeach()

	#file(INSTALL ${prereqs} DESTINATION ${_DESTINATION})
	install_resolved(FILES ${prereqs} DESTINATION "${_DESTINATION}")
endfunction()

# Like get_prerequisites, but returns full paths
function(FIND_PREREQUISITES target RESULT_VAR exclude_system recurse
	exepath dirs rpaths)
	set(RESULTS)

	get_prerequisites("${target}" _prereqs ${exclude_system} ${recurse}
                  "" "${dirs}" "${rpaths}")

	foreach(prereq ${_prereqs})
		get_filename_component(prereq_name "${prereq}" NAME)
		if(_IGNORE_CASE)
			# Windows is case insensitive.
			# Use upper characters to respect it.
			string(TOUPPER "${prereq_name}" prereq_name)
		endif()
		if("${prereq_name}" IN_LIST IGNORE_LIBS)
			continue()
		endif()

		gp_resolve_item("${LIB_DLL}" "${prereq}" "" "${dirs}" RESOLVED_PREREQ "${rpaths}")

		if(RESOLVED_PREREQ AND IS_ABSOLUTE ${RESOLVED_PREREQ} AND EXISTS ${RESOLVED_PREREQ})
			list(APPEND RESULTS ${RESOLVED_PREREQ})
		else()
			message(FATAL_ERROR "Can't resolve dependency ${prereq}.")
		endif()
	endforeach()

	set(${RESULT_VAR} ${RESULTS} PARENT_SCOPE)
endfunction()
