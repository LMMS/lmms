CMAKE_MINIMUM_REQUIRED(VERSION 3.3)

MESSAGE(STATUS "Attempting to create uninstall target for make")

#Remove all of the files listed in install_manifest.txt
SET(INSTALL_MANIFEST_PATH "${CMAKE_CURRENT_BINARY_DIR}/install_manifest.txt")
IF(NOT EXISTS ${INSTALL_MANIFEST_PATH})
	MESSAGE(FATAL_ERROR "Could not find install manifest at ${CMAKE_CURRENT_BINARY_DIR}/install_manifest.txt\nThis may be because 'make install' has not been run or install_manifest.txt has been deleted")
ENDIF()
MESSAGE(STATUS "install_manifest.txt found")
FILE(STRINGS ${INSTALL_MANIFEST_PATH} FILES_TO_REMOVE)
FOREACH(FILE_TO_REMOVE ${FILES_TO_REMOVE})
	IF(NOT EXISTS "${FILE_TO_REMOVE}")
		MESSAGE(WARNING "Could not find file ${FILE_TO_REMOVE}")
		CONTINUE()
	ENDIF()

	FILE(REMOVE "${FILE_TO_REMOVE}")
	IF(NOT EXISTS "${FILE_TO_REMOVE}")
		MESSAGE(STATUS "Successfully removed file ${FILE_TO_REMOVE}")
	ELSE()
		MESSAGE(FATAL_ERROR "Failed to remove file ${FILE_TO_REMOVE}.")
	ENDIF()
ENDFOREACH(FILE_TO_REMOVE)

# Remove empty directories created during installation

# Checks if a directory is empty and saves the result in out_var
FUNCTION(IS_EMPTY_DIR OUT_VAR DIR)
	FILE(GLOB FILES "${DIR}/*")
	LIST(LENGTH FILES NUM_FILES)
	IF(NUM_FILES EQUAL 0)
		SET(${OUT_VAR} TRUE PARENT_SCOPE)
	ELSE()
		SET(${OUT_VAR} FALSE PARENT_SCOPE)
	ENDIF()
ENDFUNCTION()

# Recursively append all parent directories of path to out_var
FUNCTION(PARENT_DIRECTORIES OUT_VAR PATH)
	GET_FILENAME_COMPONENT(PARENT "${PATH}" DIRECTORY)
	IF(PARENT AND NOT PARENT STREQUAL PATH AND NOT PARENT IN_LIST ${OUT_VAR})
		LIST(APPEND ${OUT_VAR} ${PARENT})
		PARENT_DIRECTORIES(${OUT_VAR} "${PARENT}")
	ENDIF()
	SET(${OUT_VAR} ${${OUT_VAR}} PARENT_SCOPE)
ENDFUNCTION()

# Removes all empty parent directories of the given files
FUNCTION(REMOVE_EMPTY_DIRECTORIES FILES)
	SET(DIRECTORIES)
	FOREACH(FILE_TO_REMOVE ${FILES})
		PARENT_DIRECTORIES(DIRECTORIES "${FILE_TO_REMOVE}")
	endforeach()
	LIST(REMOVE_DUPLICATES DIRECTORIES)
	# Sort and reverse so we remove subdirectories first
	LIST(SORT DIRECTORIES)
	LIST(REVERSE DIRECTORIES)

	FOREACH(DIR ${DIRECTORIES})
		# Skip directories not inside the install prefix
		IF(NOT (EXISTS "${DIR}" AND DIR MATCHES "^${CMAKE_INSTALL_PREFIX}/"))
			CONTINUE()
		ENDIF()

		IS_EMPTY_DIR(DIR_EMPTY "${DIR}")
		IF(DIR_EMPTY)
			MESSAGE(STATUS "Removing empty directory ${DIR}")
			FILE(REMOVE_RECURSE "${DIR}")
		ELSEIF()
			MESSAGE(STATUS "Skipping non-empty directory ${DIR}")
		ENDIF()
	ENDFOREACH()
ENDFUNCTION()

REMOVE_EMPTY_DIRECTORIES("${FILES_TO_REMOVE}")
