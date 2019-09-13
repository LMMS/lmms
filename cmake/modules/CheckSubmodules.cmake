# Utility for validating and, if needed, cloning all submodules
#
# Looks for a .gitmodules in the root project folder
# Loops over all modules looking well-known configure/build scripts
#
# Usage:
#       INCLUDE(CheckSubmodules)
#
# Options:
#       SET(PLUGIN_LIST "zynaddsubfx;...") # skips submodules for plugins not explicitely listed
#       SET(SKIP_SUBMODULES "plugins/Xpressive/exprtk;...") # manually skip based on full path
#
# Or via command line:       
#       cmake -DSKIP_SUBMODULES=foo;bar
#
# Copyright (c) 2017, Tres Finocchiaro, <tres.finocchiaro@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# Files which confirm a successful clone
SET(VALID_CRUMBS "CMakeLists.txt;Makefile;Makefile.in;Makefile.am;configure.ac;configure.py;autogen.sh;.gitignore;LICENSE;Home.md")

# Try and use the specified shallow clone on submodules, if supported
SET(DEPTH_VALUE 100)

# Number of times git commands will retry before failing
SET(MAX_ATTEMPTS 2)

MESSAGE("\nValidating submodules...")
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

# Assume alpha-numeric paths
STRING(REGEX MATCHALL "path = [-0-9A-Za-z/]+" SUBMODULE_LIST ${SUBMODULE_DATA})
STRING(REGEX MATCHALL "url = [.:%-0-9A-Za-z/]+" SUBMODULE_URL_LIST ${SUBMODULE_DATA})

FOREACH(_part ${SUBMODULE_LIST})
	STRING(REPLACE "path = " "" SUBMODULE_PATH ${_part})

	LIST(FIND SUBMODULE_LIST ${_part} SUBMODULE_INDEX)
	LIST(GET SUBMODULE_URL_LIST ${SUBMODULE_INDEX} _url)
	STRING(REPLACE "url = " "" SUBMODULE_URL ${_url})

	SET(SKIP false)
	SET(SKIP_REASON "(via SKIP_SUBMODULES)")
	# Remove unwanted submodules from validation by comparing against -DPLUGIN_LIST=foo;bar
	IF(${SUBMODULE_PATH} MATCHES "^plugins/")
		SET(REMOVE_PLUGIN true)
		FOREACH(_plugin ${PLUGIN_LIST})
			IF(${SUBMODULE_PATH} MATCHES "${_plugin}")
				SET(REMOVE_PLUGIN false)
			ENDIF()
		ENDFOREACH()

		IF(REMOVE_PLUGIN)
			SET(SKIP_REASON "(absent in PLUGIN_LIST)")
			LIST(APPEND SKIP_SUBMODULES ${SUBMODULE_PATH})
		ENDIF()
	ENDIF()

	# Remove submodules from validation as specified in -DSKIP_SUBMODULES=foo;bar
	IF(SKIP_SUBMODULES)
		FOREACH(_skip ${SKIP_SUBMODULES})
			IF(${SUBMODULE_PATH} MATCHES ${_skip})
				MESSAGE("-- Skipping ${SUBMODULE_PATH} matches \"${_skip}\" ${SKIP_REASON}")
				SET(SKIP true)
			ENDIF()
		ENDFOREACH()
	ENDIF()
	IF(NOT SKIP)
		LIST(INSERT SUBMODULE_LIST ${SUBMODULE_INDEX} ${SUBMODULE_PATH})
		LIST(INSERT SUBMODULE_URL_LIST ${SUBMODULE_INDEX} ${SUBMODULE_URL})
	ENDIF()
	LIST(REMOVE_ITEM SUBMODULE_LIST ${_part})
	LIST(REMOVE_ITEM SUBMODULE_URL_LIST ${_url})
ENDFOREACH()


# Once called, status is stored in GIT_RESULT respectively.
# Note: Git likes to write to stderr.  Don't assume stderr is error; Check GIT_RESULT instead.
MACRO(GIT_SUBMODULE SUBMODULE_PATH FORCE_DEINIT FORCE_REMOTE FULL_CLONE)
	FIND_PACKAGE(Git REQUIRED)
	# Handle missing commits
	SET(FORCE_REMOTE_FLAG "${FORCE_REMOTE}")
	IF(FORCE_REMOTE_FLAG)
		MESSAGE("--   Adding remote submodulefix to ${SUBMODULE_PATH}")
		EXECUTE_PROCESS(
			COMMAND ${GIT_EXECUTABLE} remote rm submodulefix
			COMMAND ${GIT_EXECUTABLE} remote add submodulefix ${FORCE_REMOTE}
			COMMAND ${GIT_EXECUTABLE} fetch submodulefix
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/${SUBMODULE_PATH}
			OUTPUT_QUIET ERROR_QUIET
		)
		# Recurse
		GIT_SUBMODULE(${SUBMODULE_PATH} false false ${FULL_CLONE})
	ELSEIF(${FORCE_DEINIT})
		MESSAGE("--   Resetting ${SUBMODULE_PATH}")
		EXECUTE_PROCESS(
			COMMAND ${GIT_EXECUTABLE} submodule deinit -f ${CMAKE_SOURCE_DIR}/${SUBMODULE_PATH}
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			OUTPUT_QUIET
		)
		# Recurse
		GIT_SUBMODULE(${SUBMODULE_PATH} false false ${FULL_CLONE})
	ELSE()
		# Try to use the depth switch
		IF(FULL_CLONE)
			# Depth doesn't revert easily... It should be "--no-recommend-shallow"
			# but it's ignored by nested submodules, use the highest value instead.
			MESSAGE("--   Fetching ${SUBMODULE_PATH}")
			SET(USE_DEPTH "2147483647")
		ELSE()
			MESSAGE("--   Fetching ${SUBMODULE_PATH} @ --depth ${DEPTH_VALUE}")
			SET(USE_DEPTH "${DEPTH_VALUE}")
		ENDIF()
		
		EXECUTE_PROCESS(
			COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive --depth ${USE_DEPTH} ${CMAKE_SOURCE_DIR}/${SUBMODULE_PATH}
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
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
	STRING(REPLACE "/" ";" PATH_PARTS ${_submodule})
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
		GIT_SUBMODULE(${_submodule} false false false)

		SET(COUNTED 0)
		SET(COUNTING "")
		# Handle edge-cases where submodule didn't clone properly or re-uses a non-empty directory
		WHILE(NOT GIT_RESULT EQUAL 0 AND COUNTED LESS MAX_ATTEMPTS)
			LIST(APPEND COUNTING "x")
			LIST(LENGTH COUNTING COUNTED)
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
					
					GIT_SUBMODULE(${_submodule} false "${SUBMODULE_URL}" false)
					BREAK()
				ELSEIF("${GIT_MESSAGE}" MATCHES "${_phrase}")
					MESSAGE("--   Retrying ${_submodule} using 'deinit' (attempt ${COUNTED} of ${MAX_ATTEMPTS})...")

					# Shallow submodules were introduced in 1.8.4
					# Shallow commits can fail to clone from non-default branches, only try once
					IF(GIT_VERSION_STRING VERSION_GREATER "1.8.3" AND COUNTED LESS 2)
						SET(FULL_CLONE false)
					ELSE()
						SET(FULL_CLONE true)
					ENDIF()
					
					GIT_SUBMODULE(${_submodule} true false ${FULL_CLONE})
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
