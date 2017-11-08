# Utility for validating and, if needed, cloning all submodules
#
# Looks for a .gitmodules in the root project folder
# Loops over all modules looking well-known configure/build scripts
#
# Usage:
#       INCLUDE(CheckSubmodules)
#
# Options:
#       SET(SKIP_SUBMODULES "foo;bar")
#
# Or via command line:       
#       cmake -DSKIP_SUBMODULES=foo;bar
#
# Copyright (c) 2017, Tres Finocchiaro, <tres.finocchiaro@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# Files which confirm a successful clone
SET(VALID_CRUMBS "CMakeLists.txt;Makefile.in;Makefile.am;configure.ac;configure.py;autogen.sh")

MESSAGE("\nValidating submodules...")
FILE(READ "${CMAKE_SOURCE_DIR}/.gitmodules" SUBMODULE_DATA)

# Assume alpha-numeric paths
STRING(REGEX MATCHALL "path = [0-9A-Za-z/]+" SUBMODULE_LIST ${SUBMODULE_DATA})
FOREACH(_part ${SUBMODULE_LIST})
	STRING(REPLACE "path = " "" SUBMODULE_PATH ${_part})

	# Remove submodules from validation as specified in -DSKIP_SUBMODULES=foo;bar
	SET(SKIP false)
	IF(SKIP_SUBMODULES)
		FOREACH(_skip ${SKIP_SUBMODULES})
			IF(${SUBMODULE_PATH} MATCHES ${_skip})
				MESSAGE("-- Skipping ${SUBMODULE_PATH} matches \"${_skip}\"")
				SET(SKIP true)
			ENDIF()
		ENDFOREACH()
	ENDIF()
	LIST(REMOVE_ITEM SUBMODULE_LIST ${_part})
	IF(NOT SKIP)
		LIST(APPEND SUBMODULE_LIST ${SUBMODULE_PATH})
	ENDIF()
ENDFOREACH()

LIST(SORT SUBMODULE_LIST)

# Attempt to do lazy clone
FOREACH(_submodule ${SUBMODULE_LIST})
	STRING(REPLACE "/" ";" PATH_PARTS ${_submodule})
	LIST(REVERSE PATH_PARTS)
	LIST(GET PATH_PARTS 0 SUBMODULE_NAME)

	MESSAGE("-- Checking ${SUBMODULE_NAME}...")
	SET(CRUMB_FOUND false)
	FOREACH(_crumb ${VALID_CRUMBS})
		IF(EXISTS "${CMAKE_SOURCE_DIR}/${_submodule}/${_crumb}")
			SET(CRUMB_FOUND true)
			MESSAGE("--   Found ${_submodule}/${_crumb}")
		ENDIF()
	ENDFOREACH()
	IF(NOT CRUMB_FOUND)
		FIND_PACKAGE(Git REQUIRED)
		MESSAGE("--   Missing ${_submodule}")
		EXECUTE_PROCESS(
			COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive ${CMAKE_SOURCE_DIR}/${_submodule}
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			ERROR_VARIABLE GIT_STDERR
		)
		IF(GIT_STDERR)
			MESSAGE(FATAL_ERROR ${GIT_STDERR})
		ENDIF()
	ENDIF()
ENDFOREACH()
MESSAGE("-- Done validating submodules.\n")
