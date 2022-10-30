# Given a library in vcpkg, find appropriate debug and release versions. If only
# one version exists, it is used as the release version, and the debug version
# is not set.
function(_get_vcpkg_library_configs _release_out _debug_out _library)
	# We want to do all operations within the vcpkg directory
	file(RELATIVE_PATH _lib_relative "${VCPKG_INSTALLED_DIR}" "${_library}")

	# Return early if we're not using vcpkg
	if(IS_ABSOLUTE _lib_relative OR _lib_relative MATCHES "^\\.\\./")
		set("${_release_out}" "${_library}" PARENT_SCOPE)
		return()
	endif()

	string(REPLACE "/" ";" _path_bits "${_lib_relative}")

	# Determine whether we were given the debug or release version
	list(FIND _path_bits "debug" _debug_index)
	if(_debug_index EQUAL -1)
		# We have the release version, so use it
		set(_release_lib "${_library}")

		# Try to find a debug version too
		list(FIND _path_bits "lib" _lib_index)
		if(_lib_index GREATER_EQUAL 0)
			list(INSERT _path_bits "${_lib_index}" "debug")
			list(INSERT _path_bits 0 "${VCPKG_INSTALLED_DIR}")
			string(REPLACE ";" "/" _debug_lib "${_path_bits}")

			if(NOT EXISTS "${_debug_lib}")
				# Debug version does not exist - only use given version
				unset(_debug_lib)
			endif()
		endif()
	else()
		# We have the debug version, so try to find a release version too
		list(REMOVE_AT _path_bits "${_debug_index}")
		list(INSERT _path_bits 0 "${VCPKG_INSTALLED_DIR}")
		string(REPLACE ";" "/" _release_lib "${_path_bits}")

		if(NOT EXISTS "${_release_lib}")
			# Release version does not exist - only use given version
			set(_release_lib "${_library}")
		else()
			# Release version exists, so use given version as debug
			set(_debug_lib "${_library}")
		endif()
	endif()

	# Set output variables appropriately
	if(_debug_lib)
		set("${_release_out}" "${_release_lib}" PARENT_SCOPE)
		set("${_debug_out}" "${_debug_lib}" PARENT_SCOPE)
	else()
		set("${_release_out}" "${_release_lib}" PARENT_SCOPE)
		unset("${_debug_out}" PARENT_SCOPE)
	endif()
endfunction()
