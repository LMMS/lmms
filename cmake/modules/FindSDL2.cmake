# This module defines
# SDL2::SDL2, a target providing SDL2 itself
# SDL2_LIBRARY, the name of the library to link against
# SDL2_FOUND, if false, do not try to link to SDL2
# SDL2_INCLUDE_DIR, where to find SDL.h
#
# On OSX, this will prefer the Framework version (if found) over others.
# People will have to manually change the cache values of
# SDL2_LIBRARY to override this selection or set the CMake environment
# CMAKE_INCLUDE_PATH to modify the search paths.
#
# $SDL2DIR is an environment variable that would
# correspond to the ./configure --prefix=$SDL2DIR
# used in building SDL2.
#
# Modified by Eric Wing, l.e.galup, and Dominic Clark
#
#=============================================================================
# Copyright 2003-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file COPYING-CMAKE-SCRIPTS for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

# Try config mode first - anything SDL2 itself provides is likely to be more
# reliable than our guesses.
find_package(SDL2 CONFIG QUIET)

if(TARGET SDL2::SDL2)
	# Extract details for find_package_handle_standard_args
	get_target_property(SDL2_LIBRARY SDL2::SDL2 LOCATION)
	get_target_property(SDL2_INCLUDE_DIR SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)
else()
	set(SDL2_SEARCH_PATHS
		~/Library/Frameworks
		/Library/Frameworks
		/usr/local
		/usr
		/sw # Fink
		/opt/local # DarwinPorts
		/opt/csw # Blastwave
		/opt
		${SDL2_PATH}
	)

	find_path(SDL2_INCLUDE_DIR
		NAMES SDL.h
		HINTS $ENV{SDL2DIR} ${SDL2_INCLUDE_DIRS}
		PATH_SUFFIXES SDL2 include/SDL2 include
		PATHS ${SDL2_SEARCH_PATHS}
	)

	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(PATH_SUFFIXES lib64 lib/x64 lib)
	else()
		set(PATH_SUFFIXES lib/x86 lib)
	endif()

	find_library(SDL2_LIBRARY
		NAMES SDL2
		HINTS $ENV{SDL2DIR} ${SDL2_LIBDIR}
		PATH_SUFFIXES ${PATH_SUFFIXES}
		PATHS ${SDL2_SEARCH_PATHS}
	)

	# SDL2 may require threads on your system.
	# The Apple build may not need an explicit flag because one of the
	# frameworks may already provide it.
	# But for non-OSX systems, I will use the CMake Threads package.
	if(NOT APPLE)
		find_package(Threads)
	endif()

	if(SDL2_LIBRARY AND SDL2_INCLUDE_DIR)
		add_library(SDL2::SDL2 UNKNOWN IMPORTED)
		set_target_properties(SDL2::SDL2 PROPERTIES
			IMPORTED_LOCATION "${SDL2_LIBRARY}"
			INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIR}"
		)

		# For OS X, SDL2 uses Cocoa as a backend so it must link to Cocoa.
		if(APPLE)
			set_property(TARGET SDL2::SDL2 APPEND PROPERTY
				INTERFACE_LINK_OPTIONS "-framework Cocoa"
			)
		endif()

		# For threads, as mentioned Apple doesn't need this.
		# In fact, there seems to be a problem if I used the Threads package
		# and try using this line, so I'm just skipping it entirely for OS X.
		if(NOT APPLE AND Threads_FOUND)
			set_property(TARGET SDL2::SDL2 APPEND PROPERTY
				INTERFACE_LINK_LIBRARIES "Threads::Threads"
			)
		endif()

		if(EXISTS "${SDL2_INCLUDE_DIR}/SDL_version.h")
			file(READ "${SDL2_INCLUDE_DIR}/SDL_version.h" _sdl_version_h)
			string(REGEX REPLACE ".*#[\t ]*define[\t ]+SDL_MAJOR_VERSION[\t ]+([0-9]+).*" "\\1" SDL2_VERSION_MAJOR "${_sdl_version_h}")
			string(REGEX REPLACE ".*#[\t ]*define[\t ]+SDL_MINOR_VERSION[\t ]+([0-9]+).*" "\\1" SDL2_VERSION_MINOR "${_sdl_version_h}")
			string(REGEX REPLACE ".*#[\t ]*define[\t ]+SDL_PATCHLEVEL[\t ]+([0-9]+).*" "\\1" SDL2_VERSION_PATCH "${_sdl_version_h}")
			set(SDL2_VERSION "${SDL2_VERSION_MAJOR}.${SDL2_VERSION_MINOR}.${SDL2_VERSION_PATCH}")
			unset(_sdl_version_h)
		endif()
	endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL2
	REQUIRED_VARS SDL2_LIBRARY SDL2_INCLUDE_DIR
	VERSION_VAR SDL2_VERSION
)
