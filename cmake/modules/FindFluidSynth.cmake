# Copyright (c) 2022 Dominic Clark
#
# Redistribution and use is allowed according to the terms of the New BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# Return if we already have FluidSynth
if(TARGET fluidsynth)
	set(FluidSynth_FOUND 1)
	return()
endif()

# Attempt to find FluidSynth using PkgConfig
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
	pkg_check_modules(FLUIDSYNTH_PKG fluidsynth)
endif()

# Find the library and headers using the results from PkgConfig as a guide
find_path(FluidSynth_INCLUDE_DIR
	NAMES "fluidsynth.h"
	HINTS ${FLUIDSYNTH_PKG_INCLUDE_DIRS}
)

find_library(FluidSynth_LIBRARY
	NAMES "fluidsynth"
	HINTS ${FLUIDSYNTH_PKG_LIBRARY_DIRS}
)

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

if(FluidSynth_INCLUDE_DIR AND FluidSynth_LIBRARY)
	add_library(fluidsynth SHARED IMPORTED)
	set_target_properties(fluidsynth PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${FluidSynth_INCLUDE_DIR}"
	)
	if(WIN32)
		if(VCPKG_INSTALLED_DIR)
			_get_vcpkg_library_configs(FluidSynth_IMPLIB_RELEASE FluidSynth_IMPLIB_DEBUG "${FluidSynth_LIBRARY}")
		else()
			set(FluidSynth_IMPLIB_RELEASE "${FluidSynth_LIBRARY}")
		endif()
		if(FluidSynth_IMPLIB_DEBUG)
			set_target_properties(fluidsynth PROPERTIES
				IMPORTED_IMPLIB_RELEASE "${FluidSynth_IMPLIB_RELEASE}"
				IMPORTED_IMPLIB_DEBUG "${FluidSynth_IMPLIB_DEBUG}"
			)
		else()
			set_target_properties(fluidsynth PROPERTIES
				IMPORTED_IMPLIB "${FluidSynth_IMPLIB_RELEASE}"
			)
		endif()
	else()
		set_target_properties(fluidsynth PROPERTIES
			IMPORTED_LOCATION "${FluidSynth_LIBRARY}"
		)
	endif()

	if(EXISTS "${FluidSynth_INCLUDE_DIR}/fluidsynth/version.h")
		file(STRINGS
			"${FluidSynth_INCLUDE_DIR}/fluidsynth/version.h"
			_version_string
			REGEX "^#[\t ]*define[\t ]+FLUIDSYNTH_VERSION[\t ]+\".*\""
		)
		string(REGEX REPLACE
			"^.*FLUIDSYNTH_VERSION[\t ]+\"([^\"]*)\".*$"
			"\\1"
			FluidSynth_VERSION_STRING
			"${_version_string}"
		)
		unset(_version_string)
	endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FluidSynth
	REQUIRED_VARS FluidSynth_LIBRARY FluidSynth_INCLUDE_DIR
	VERSION_VAR FluidSynth_VERSION_STRING
)
