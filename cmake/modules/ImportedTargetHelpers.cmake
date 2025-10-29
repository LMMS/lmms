# ImportedTargetHelpers.cmake - various helper functions for use in find modules.
#
# Copyright (c) 2022-2023 Dominic Clark
#
# Redistribution and use is allowed according to the terms of the New BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# If the version variable is not yet set, build the source linked to the target,
# run it, and set the version variable to the output. Useful for libraries which
# do not expose the version information in a header where it can be extracted
# with regular expressions, but do provide a function to get the version.
#
# Usage:
# 	determine_version_from_source(
#		<output variable> # The cache variable in which to store the computed version
#		<target>          # The target which the source will link to
#		<source>          # The source code to determine the version
#	)
function(determine_version_from_source _version_out _target _source)
	# Return if we already know the version, or the target was not found
	if(NOT "${${_version_out}}" STREQUAL "" OR NOT TARGET "${_target}")
		return()
	endif()

	# Return with a notice if cross-compiling, since we are unlikely to be able
	# to run the compiled source
	if(CMAKE_CROSSCOMPILING)
		message(
			"${_target} was found but the version could not be determined automatically.\n"
			"Set the cache variable `${_version_out}` to the version you have installed."
		)
		return()
	endif()

	# Write the source code to a temporary file
	string(SHA1 _source_hash "${_source}")
	set(_source_file "${CMAKE_CURRENT_BINARY_DIR}/${_source_hash}.cpp")
	file(WRITE "${_source_file}" "${_source}")

	# Build and run the temporary file to get the version
	# TODO CMake 3.25: Use the new signature for try_run which has a NO_CACHE
	#                  option and doesn't require separate file management.
	try_run(
		_dvfs_run_result _dvfs_compile_result "${CMAKE_CURRENT_BINARY_DIR}"
		SOURCES "${_source_file}"
		LINK_LIBRARIES "${_target}"
		CXX_STANDARD 20
		RUN_OUTPUT_VARIABLE _run_output
		COMPILE_OUTPUT_VARIABLE _compile_output
	)

	# Clean up the temporary file
	file(REMOVE "${_source_file}")

	# Set the version if the run was successful, using a cache variable since
	# this version check may be relatively expensive. Otherwise, log the error
	# and inform the user.
	if(_dvfs_run_result EQUAL "0")
		set("${_version_out}" "${_run_output}" CACHE INTERNAL "Version of ${_target}")
	else()
		message(DEBUG "${_compile_output}")
		message(
			"${_target} was found but the version could not be determined automatically.\n"
			"Set the cache variable `${_version_out}` to the version you have installed."
		)
	endif()
endfunction()

# Search for a package using config mode. If this fails to find the desired
# target, use the specified fallbacks and add the target if they succeed. Set
# the variables `prefix_LIBRARY`, `prefix_INCLUDE_DIRS`, and `prefix_VERSION`
# if found for the caller to pass to `find_package_handle_standard_args`.
#
# Usage:
# 	find_package_config_mode_with_fallback(
#		<package_name>                 # The package to search for with config mode
#		<target_name>                  # The target to expect from config mode, or define if not found
# 		LIBRARY_NAMES names...         # Possible library names to search for as a fallback
# 		INCLUDE_NAMES names...         # Possible header names to search for as a fallback
# 		[PKG_CONFIG <pkg_config_name>] # The pkg-config name to search for as a fallback
# 		[LIBRARY_HINTS hints...]       # Locations to look for libraries
# 		[INCLUDE_HINTS hints...]       # Locations to look for headers
#		[DEPENDS dependencies...]      # Dependencies of the target - added to INTERFACE_LINK_LIBRARIES, and will fail if not found
#		[PREFIX <prefix>]              # The prefix for result variables - defaults to the package name
# 	)
function(find_package_config_mode_with_fallback _fpcmwf_PACKAGE_NAME _fpcmwf_TARGET_NAME)
	# Parse remaining arguments
	set(_options "")
	set(_one_value_args "PKG_CONFIG" "PREFIX")
	set(_multi_value_args "LIBRARY_NAMES" "LIBRARY_HINTS" "INCLUDE_NAMES" "INCLUDE_HINTS" "DEPENDS")
	cmake_parse_arguments(PARSE_ARGV 2 _fpcmwf "${_options}" "${_one_value_args}" "${_multi_value_args}")

	# Compute result variable names
	if(NOT DEFINED _fpcmwf_PREFIX)
		set(_fpcmwf_PREFIX "${_fpcmwf_PACKAGE_NAME}")
	endif()
	set(_version_var "${_fpcmwf_PREFIX}_VERSION")
	set(_library_var "${_fpcmwf_PREFIX}_LIBRARY")
	set(_include_var "${_fpcmwf_PREFIX}_INCLUDE_DIRS")

	# Try config mode if possible
	find_package("${_fpcmwf_PACKAGE_NAME}" CONFIG QUIET)

	if(TARGET "${_fpcmwf_TARGET_NAME}")
		# Extract package details from existing target
		get_target_property("${_library_var}" "${_fpcmwf_TARGET_NAME}" LOCATION)
		get_target_property("${_include_var}" "${_fpcmwf_TARGET_NAME}" INTERFACE_INCLUDE_DIRECTORIES)
		if(DEFINED "${_fpcmwf_PACKAGE_NAME}_VERSION")
			set("${_version_var}" "${${_fpcmwf_PACKAGE_NAME}_VERSION}")
		endif()
	else()
		# Check whether the dependencies exist
		foreach(_dependency IN LISTS _fpcmwf_DEPENDS)
			if(NOT TARGET "${_dependency}")
				return()
			endif()
		endforeach()

		# Attempt to find the package using pkg-config, if we have it and it was requested
		set(_pkg_config_prefix "${_fpcmwf_PKG_CONFIG}_PKG")
		if(DEFINED _fpcmwf_PKG_CONFIG)
			find_package(PkgConfig QUIET)
			if(PKG_CONFIG_FOUND)
				pkg_check_modules("${_pkg_config_prefix}" QUIET "${_fpcmwf_PKG_CONFIG}")
				if("${${_pkg_config_prefix}_FOUND}")
					set("${_version_var}" "${${_pkg_config_prefix}_VERSION}")
				endif()
			endif()
		endif()

		# Find the library and headers using the results from pkg-config as a guide
		find_library("${_library_var}"
			NAMES ${_fpcmwf_LIBRARY_NAMES}
			HINTS ${${_pkg_config_prefix}_LIBRARY_DIRS} ${_fpcmwf_LIBRARY_HINTS}
		)

		find_path("${_include_var}"
			NAMES ${_fpcmwf_INCLUDE_NAMES}
			HINTS ${${_pkg_config_prefix}_INCLUDE_DIRS} ${_fpcmwf_INCLUDE_HINTS}
		)

		# Create an imported target if we succeeded in finding the package
		if(${_library_var} AND ${_include_var})
			add_library("${_fpcmwf_TARGET_NAME}" UNKNOWN IMPORTED)
			set_target_properties("${_fpcmwf_TARGET_NAME}" PROPERTIES
				IMPORTED_LOCATION "${${_library_var}}"
				INTERFACE_INCLUDE_DIRECTORIES "${${_include_var}}"
				INTERFACE_LINK_LIBRARIES "${_fpcmwf_DEPENDS}"
			)
		endif()

		mark_as_advanced("${_library_var}" "${_include_var}")
	endif()

	# Return results to caller
	if(DEFINED "${_version_var}")
		set("${_version_var}" "${${_version_var}}" PARENT_SCOPE)
	else()
		unset("${_version_var}" PARENT_SCOPE)
	endif()
	set("${_library_var}" "${${_library_var}}" PARENT_SCOPE)
	set("${_include_var}" "${${_include_var}}" PARENT_SCOPE)
endfunction()

# Given a library in vcpkg, find appropriate debug and release versions. If only
# one version exists, use it as the release version, and do not set the debug
# version.
#
# Usage:
#	get_vcpkg_library_configs(
#		<release library> # Variable in which to store the path to the release version of the library
#		<debug library>   # Variable in which to store the path to the debug version of the library
#		<base library>    # Known path to some version of the library
#	)
function(get_vcpkg_library_configs _release_out _debug_out _library)
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
