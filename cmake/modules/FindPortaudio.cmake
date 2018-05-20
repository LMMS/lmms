# - Try to find Portaudio
# Once done this will define
#
#  PORTAUDIO_FOUND - system has Portaudio
#  PORTAUDIO_INCLUDE_DIRS - the Portaudio include directory
#  PORTAUDIO_LIBRARIES - Link these to use Portaudio
#  PORTAUDIO_DEFINITIONS - Compiler switches required for using Portaudio
#
#  Copyright (c) 2006 Andreas Schneider <mail@cynapses.org>
#
# Redistribution and use is allowed according to the terms of the New BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


IF(PORTAUDIO_LIBRARIES AND PORTAUDIO_INCLUDE_DIRS)
	# in cache already
	set(PORTAUDIO_FOUND TRUE)
ELSE(PORTAUDIO_LIBRARIES AND PORTAUDIO_INCLUDE_DIRS)
	IF(USING_VCPKG)
		FIND_FILE(PORTAUDIO_INCLUDE_DIRS "portaudio.h")
		FIND_LIBRARY(PORTAUDIO_LIBRARIES "portaudio.lib")
		IF(PORTAUDIO_INCLUDE_DIRS)
			SET(PORTAUDIO_FOUND TRUE)
		ENDIF(PORTAUDIO_INCLUDE_DIRS)
	ELSE(USING_VCPKG)
		include(FindPkgConfig)
		pkg_check_modules(PORTAUDIO portaudio-2.0)
		if (PORTAUDIO_FOUND)
		if (NOT Portaudio_FIND_QUIETLY)
			message(STATUS "Found Portaudio: ${PORTAUDIO_LIBRARIES}")
		endif (NOT Portaudio_FIND_QUIETLY)
		else (PORTAUDIO_FOUND)
		if (Portaudio_FIND_REQUIRED)
			message(FATAL_ERROR "Could not find Portaudio")
		endif (Portaudio_FIND_REQUIRED)
		endif (PORTAUDIO_FOUND)

		# show the PORTAUDIO_INCLUDE_DIRS and PORTAUDIO_LIBRARIES variables only in the advanced view
		mark_as_advanced(PORTAUDIO_INCLUDE_DIRS PORTAUDIO_LIBRARIES)
	ENDIF(USING_VCPKG)
ENDIF(PORTAUDIO_LIBRARIES AND PORTAUDIO_INCLUDE_DIRS)

