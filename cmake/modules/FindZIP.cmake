# - Find libzip
# Find the native libzip includes and library
#
#  ZIP_INCLUDE_DIRS - where to find zip.h
#  ZIP_LIBRARIES    - List of libraries when using libzip.
#  ZIP_FOUND        - True if libzip found.

FIND_PATH(ZIP_INCLUDE_DIRS zip.h /usr/include)
FIND_LIBRARY(ZIP_LIBRARIES NAMES zip PATH /usr/lib /usr/local/lib)

IF(ZIP_INCLUDE_DIRS AND ZIP_LIBRARIES)
	SET(ZIP_FOUND TRUE)
ENDIF(ZIP_INCLUDE_DIRS AND ZIP_LIBRARIES)

IF(ZIP_FOUND)
	IF(NOT ZIP_FIND_QUIETLY)
		MESSAGE(STATUS "Found libzip: ${ZIP_LIBRARIES}")
	ENDIF(NOT ZIP_FIND_QUIETLY) 
ELSE(ZIP_FOUND)
	SET(ZIP_LIBRARIES "")
	SET(ZIP_INCLUDE_DIRS "")
	MESSAGE(STATUS "Could not find libzip")
ENDIF(ZIP_FOUND)

MARK_AS_ADVANCED( ZIP_LIBRARIES ZIP_INCLUDE_DIRS )
