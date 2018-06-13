include(GetPrerequisites)
include(CMakeParseArguments)

function(make_absolute var)
	get_filename_component(abs "${${var}}" ABSOLUTE BASE_DIR "${CMAKE_INSTALL_PREFIX}")
	set(${var} ${abs} PARENT_SCOPE)
endfunction()

function(make_all_absolute list_var)
	set(result "")
	foreach(file ${${list_var}})
		make_absolute(file)
		list(APPEND result ${file})
	endforeach()
	set(${list_var} ${result} PARENT_SCOPE)
endfunction()

function(INSTALL_DEPENDENCIES)
	cmake_parse_arguments("" "" "DESTINATION" "FILES;LIB_DIRS;SEARCH_PATHS" ${ARGN})

	# Make paths absolute
	make_absolute(_DESTINATION)
	make_all_absolute(_FILES)
	make_all_absolute(_LIB_DIRS)
	make_all_absolute(_SEARCH_PATHS)

	set(prereqs "")
	foreach(file ${_FILES})

		get_filename_component(file_name "${file}" NAME)
		message(STATUS "Finding prerequisites of ${file_name}")
		find_prerequisites("${file}" _prereqs
			1 # exclude system files
			1 # recurse
			""
			"${_LIB_DIRS}"
			"${_SEARCH_PATHS}"
		)

		list(APPEND prereqs ${_prereqs})
	endforeach()

	list(REMOVE_DUPLICATES prereqs)

	foreach(prereq ${prereqs})
		get_filename_component(prereq_name "${prereq}" NAME)
		foreach(rpath ${_SEARCH_PATHS})
			if(EXISTS "${rpath}/${prereq_name}")
				list(REMOVE_ITEM prereqs "${prereq}")
			endif()
		endforeach()
	endforeach()

	file(INSTALL ${prereqs} DESTINATION ${_DESTINATION})
endfunction()

function(FIND_PREREQUISITES target RESULT_VAR exclude_system recurse
	exepath dirs rpaths)
	set(RESULTS)

	get_prerequisites("${target}" _prereqs ${exclude_system} ${recurse}
                  "" "${dirs}" "${rpaths}")

	foreach(PREREQ ${_prereqs})
		gp_resolve_item("${LIB_DLL}" "${PREREQ}" "" "${dirs}" RESOLVED_PREREQ "${rpaths}")
		if(NOT RESOLVED_PREREQ)
			message(SEND_ERROR "Can't resolve dependency ${PREREQ}.")
		endif()
		list(APPEND RESULTS ${RESOLVED_PREREQ})
	endforeach()

	set(${RESULT_VAR} ${RESULTS} PARENT_SCOPE)
endfunction()
