# Copyright (c) 2022 Dominic Clark
#
# Redistribution and use is allowed according to the terms of the New BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# Return if we already have PortAudio
if(TARGET portaudio)
	set(Portaudio_FOUND 1)
	return()
endif()

# Attempt to find PortAudio using PkgConfig, if we have it
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
	pkg_check_modules(PORTAUDIO_PKG portaudio-2.0)
endif()

# Find the library and headers using the results from PkgConfig as a guide
find_library(Portaudio_LIBRARY
	NAMES "portaudio"
	HINTS ${PORTAUDIO_PKG_LIBRARY_DIRS}
)

find_path(Portaudio_INCLUDE_DIR
	NAMES "portaudio.h"
	HINTS ${PORTAUDIO_PKG_INCLUDE_DIRS}
)

# Create an imported target for PortAudio if we succeeded in finding it.
if(Portaudio_LIBRARY AND Portaudio_INCLUDE_DIR)
	add_library(portaudio UNKNOWN IMPORTED)
	set_target_properties(portaudio PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${Portaudio_INCLUDE_DIR}"
		IMPORTED_LOCATION "${Portaudio_LIBRARY}"
	)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Portaudio
	REQUIRED_VARS Portaudio_LIBRARY Portaudio_INCLUDE_DIR
)
