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
	NAMES "fluidsynth" "fluidsynth-3" "fluidsynth-2" "fluidsynth-1"
	HINTS ${FLUIDSYNTH_PKG_LIBRARY_DIRS}
)

if(FluidSynth_INCLUDE_DIR AND FluidSynth_LIBRARY)
	add_library(fluidsynth UNKNOWN IMPORTED)
	set_target_properties(fluidsynth PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${FluidSynth_INCLUDE_DIR}"
	)

	if(VCPKG_INSTALLED_DIR)
		include(ImportedTargetHelpers)
		get_vcpkg_library_configs(FluidSynth_IMPLIB_RELEASE FluidSynth_IMPLIB_DEBUG "${FluidSynth_LIBRARY}")
	else()
		set(FluidSynth_IMPLIB_RELEASE "${FluidSynth_LIBRARY}")
	endif()

	if(FluidSynth_IMPLIB_DEBUG)
		set_target_properties(fluidsynth PROPERTIES
			IMPORTED_LOCATION_RELEASE "${FluidSynth_IMPLIB_RELEASE}"
			IMPORTED_LOCATION_DEBUG "${FluidSynth_IMPLIB_DEBUG}"
		)
	else()
		set_target_properties(fluidsynth PROPERTIES
			IMPORTED_LOCATION "${FluidSynth_IMPLIB_RELEASE}"
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
