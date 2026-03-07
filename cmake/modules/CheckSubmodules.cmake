# Utility for validating and, if needed, cloning all submodules
#
# Looks for a .gitmodules in the root project folder
# Loops over all modules looking well-known configure/build scripts
#
# Usage:
#       INCLUDE(CheckSubmodules)
#
# Options:
#       SET(PLUGIN_LIST "ZynAddSubFx;...") # skips submodules for plugins not explicitly listed
#
# Or via command line:       
#       cmake -PLUGIN_LIST=foo;bar
#
# Copyright (c) 2019, Tres Finocchiaro, <tres.finocchiaro@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# Files which confirm a successful clone
SET(VALID_CRUMBS "CMakeLists.txt;Makefile;Makefile.in;Makefile.am;configure.ac;configure.py;autogen.sh;.gitignore;LICENSE;Home.md;license.txt")

OPTION(NO_SHALLOW_CLONE "Disable shallow cloning of submodules" OFF)

# Try and use the specified shallow clone on submodules, if supported
SET(DEPTH_VALUE 100)

# Number of times git commands will retry before failing
SET(MAX_ATTEMPTS 2)

MESSAGE("\nChecking submodules...")
IF(NOT EXISTS "${CMAKE_SOURCE_DIR}/.gitmodules")
	MESSAGE("Skipping the check because .gitmodules not detected."
		"Please make sure you have all submodules in the source tree!"
	)
	RETURN()
ENDIF()
FILE(READ "${CMAKE_SOURCE_DIR}/.gitmodules" SUBMODULE_DATA)

# Force English locale
SET(LC_ALL_BACKUP "$ENV{LC_ALL}")
SET(LANG_BACKUP "$ENV{LANG}")
SET(ENV{LC_ALL} "C")
SET(ENV{LANG} "en_US")

# Submodule list pairs, unparsed (WARNING: Assumes alphanumeric paths)
STRING(REGEX MATCHALL "path = [-0-9A-Za-z/]+" SUBMODULE_LIST_RAW ${SUBMODULE_DATA})
STRING(REGEX MATCHALL "url = [.:%-0-9A-Za-z/]+" SUBMODULE_URL_RAW ${SUBMODULE_DATA})

# Submodule list pairs, parsed
SET(SUBMODULE_LIST "")
SET(SUBMODULE_URL "")

FOREACH(_path ${SUBMODULE_LIST_RAW})
	# Parse SUBMODULE_PATH
	STRING(REPLACE "path = " "" SUBMODULE_PATH "${_path}")

	# Grab index for matching SUBMODULE_URL
	LIST(FIND SUBMODULE_LIST_RAW "${_path}" SUBMODULE_INDEX)
	LIST(GET SUBMODULE_URL_RAW ${SUBMODULE_INDEX} _url)

	# Parse SUBMODULE_URL
	STRING(REPLACE "url = " "" SUBMODULE_URL "${_url}")

	SET(SKIP false)

	# Loop over skipped plugins, add to SKIP_SUBMODULES (e.g. -DPLUGIN_LIST=foo;bar)
	IF(${SUBMODULE_PATH} MATCHES "^plugins/")
		SET(REMOVE_PLUGIN true)
		FOREACH(_plugin ${PLUGIN_LIST})
			IF(_plugin STREQUAL "")
				CONTINUE()
			ENDIF()
			IF(${SUBMODULE_PATH} MATCHES "${_plugin}")
				SET(REMOVE_PLUGIN false)
			ENDIF()
		ENDFOREACH()

		IF(REMOVE_PLUGIN)
			LIST(APPEND SKIP_SUBMODULES "${SUBMODULE_PATH}")
		ENDIF()
	ENDIF()

	# Finally, loop and mark "SKIP" on match
	IF(SKIP_SUBMODULES)
		FOREACH(_skip ${SKIP_SUBMODULES})
			IF("${SUBMODULE_PATH}" MATCHES "${_skip}")
				MESSAGE("-- Skipping ${SUBMODULE_PATH} matches \"${_skip}\" (absent in PLUGIN_LIST)")
				SET(SKIP true)
				BREAK()
			ENDIF()
		ENDFOREACH()
	ENDIF()

	IF(NOT SKIP)
		LIST(APPEND SUBMODULE_LIST "${SUBMODULE_PATH}")
		LIST(APPEND SUBMODULE_URL "${SUBMODULE_URL}")
	ENDIF()
ENDFOREACH()

# Once called, status is stored in GIT_RESULT respectively.
# Note: Git likes to write to stderr.  Don't assume stderr is error; Check GIT_RESULT instead.
MACRO(GIT_SUBMODULE SUBMODULE_PATH FORCE_DEINIT FORCE_REMOTE NO_DEPTH)
	FIND_PACKAGE(Git REQUIRED)
	# Handle missing commits
	SET(FORCE_REMOTE_FLAG "${FORCE_REMOTE}")
	SET(NO_DEPTH_FLAG "${NO_DEPTH}")
	IF(FORCE_REMOTE_FLAG)
		MESSAGE("--   Adding remote submodulefix to ${SUBMODULE_PATH}")
		EXECUTE_PROCESS(
			COMMAND "${GIT_EXECUTABLE}" remote rm submodulefix
			COMMAND "${GIT_EXECUTABLE}" remote add submodulefix ${FORCE_REMOTE}
			COMMAND "${GIT_EXECUTABLE}" fetch submodulefix
			WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/${SUBMODULE_PATH}"
			OUTPUT_QUIET ERROR_QUIET
		)
		# Recurse
		GIT_SUBMODULE(${SUBMODULE_PATH} false false ${NO_DEPTH_FLAG})
	ELSEIF(${FORCE_DEINIT})
		MESSAGE("--   Resetting ${SUBMODULE_PATH}")
		EXECUTE_PROCESS(
			COMMAND "${GIT_EXECUTABLE}" submodule deinit -f "${CMAKE_SOURCE_DIR}/${SUBMODULE_PATH}"
			WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
			OUTPUT_QUIET
		)
		MESSAGE("--   Deleting ${CMAKE_SOURCE_DIR}/.git/${SUBMODULE_PATH}")
		FILE(REMOVE_RECURSE "${CMAKE_SOURCE_DIR}/.git/modules/${SUBMODULE_PATH}")
		# Recurse without depth
		GIT_SUBMODULE(${SUBMODULE_PATH} false false true)
	ELSE()
		# Try to use the depth switch
		IF(NO_SHALLOW_CLONE OR GIT_VERSION_STRING VERSION_LESS "1.8.4" OR NO_DEPTH_FLAG)
			# Shallow submodules were introduced in 1.8.4
			MESSAGE("--   Fetching ${SUBMODULE_PATH}")
			SET(DEPTH_CMD "")
			SET(DEPTH_VAL "")
		ELSE()
			MESSAGE("--   Fetching ${SUBMODULE_PATH} @ --depth ${DEPTH_VALUE}")
			SET(DEPTH_CMD "--depth")
			SET(DEPTH_VAL "${DEPTH_VALUE}")
		ENDIF()
		
		EXECUTE_PROCESS(
			COMMAND "${GIT_EXECUTABLE}" submodule update --init --recursive ${DEPTH_CMD} ${DEPTH_VAL} "${CMAKE_SOURCE_DIR}/${SUBMODULE_PATH}"
			WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
			RESULT_VARIABLE GIT_RESULT
			OUTPUT_VARIABLE GIT_STDOUT
			ERROR_VARIABLE GIT_STDERR
		)

		SET(GIT_MESSAGE "${GIT_STDOUT}${GIT_STDERR}")
		MESSAGE("${GIT_MESSAGE}")
	ENDIF()
ENDMACRO()

SET(MISSING_COMMIT_PHRASES "no such remote ref;reference is not a tree;unadvertised object")
SET(RETRY_PHRASES "Failed to recurse;cannot create directory;already exists;${MISSING_COMMIT_PHRASES}")

# Attempt to do lazy clone
FOREACH(_submodule ${SUBMODULE_LIST})
	STRING(REPLACE "/" ";" PATH_PARTS "${_submodule}")
	LIST(REVERSE PATH_PARTS)
	LIST(GET PATH_PARTS 0 SUBMODULE_NAME)

	MESSAGE("-- Checking ${SUBMODULE_NAME}...")
	SET(CRUMB_FOUND false)
	FOREACH(_crumb ${VALID_CRUMBS})
		IF(EXISTS "${CMAKE_SOURCE_DIR}/${_submodule}/${_crumb}")
			SET(CRUMB_FOUND true)
			MESSAGE("--   Found ${_submodule}/${_crumb}")
			BREAK()
		ENDIF()
	ENDFOREACH()
	IF(NOT CRUMB_FOUND)
		GIT_SUBMODULE("${_submodule}" false false false)

		SET(COUNTED 0)
		# Handle edge-cases where submodule didn't clone properly or re-uses a non-empty directory
		WHILE(NOT GIT_RESULT EQUAL 0 AND COUNTED LESS MAX_ATTEMPTS)
			MATH(EXPR COUNTED "${COUNTED}+1")
			SET(MISSING_COMMIT false)
			FOREACH(_phrase ${MISSING_COMMIT_PHRASES})
				IF("${GIT_MESSAGE}" MATCHES "${_phrase}")
					SET(MISSING_COMMIT true)
					BREAK()
				ENDIF()
			ENDFOREACH()
			FOREACH(_phrase ${RETRY_PHRASES})
				IF(${MISSING_COMMIT} AND COUNTED LESS 2)
					LIST(FIND SUBMODULE_LIST ${_submodule} SUBMODULE_INDEX)
					LIST(GET SUBMODULE_URL_LIST ${SUBMODULE_INDEX} SUBMODULE_URL)
					MESSAGE("--   Retrying ${_submodule} using 'remote add submodulefix' (attempt ${COUNTED} of ${MAX_ATTEMPTS})...")
					
					GIT_SUBMODULE("${_submodule}" false "${SUBMODULE_URL}" false)
					BREAK()
				ELSEIF("${GIT_MESSAGE}" MATCHES "${_phrase}")
					MESSAGE("--   Retrying ${_submodule} using 'deinit' (attempt ${COUNTED} of ${MAX_ATTEMPTS})...")
					GIT_SUBMODULE("${_submodule}" true false false)
					BREAK()
				ENDIF()
			ENDFOREACH()
		ENDWHILE()

		IF(NOT GIT_RESULT EQUAL 0)
			MESSAGE(FATAL_ERROR "${GIT_EXECUTABLE} exited with status of ${GIT_RESULT}")
		ENDIF()
	ENDIF()
ENDFOREACH()
MESSAGE("-- Done validating submodules.\n")

# Reset locale
SET(ENV{LC_ALL} "${LC_ALL_BACKUP}")
SET(ENV{LANG} "${LANG_BACKUP}")
