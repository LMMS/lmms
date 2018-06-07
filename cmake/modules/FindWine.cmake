# - Try to find the wine libraries
# Once done this will define
#
#  WINE_FOUND - System has wine
#  WINE_INCLUDE_DIRS - The wine include directories
#  WINE_LIBRARIES - The libraries needed to use wine
#  WINE_DEFINITIONS - Compiler switches required for using wine
#

LIST(APPEND CMAKE_PREFIX_PATH /opt/wine-stable /opt/wine-devel /opt/wine-staging /usr/lib/wine/)

FIND_PATH(WINE_INCLUDE_DIR wine/exception.h PATH_SUFFIXES wine)
FIND_LIBRARY(WINE_LIBRARY NAMES wine PATH_SUFFIXES wine i386-linux-gnu/wine)
FIND_PROGRAM(WINE_CXX
	NAMES wineg++ winegcc winegcc64 winegcc32 winegcc-stable
	PATHS /usr/lib/wine)
FIND_PROGRAM(WINE_BUILD NAMES winebuild)

SET(WINE_INCLUDE_DIRS ${WINE_INCLUDE_DIR} )
SET(WINE_LIBRARIES ${WINE_LIBRARY} )

# Handle wine linking problems
EXEC_PROGRAM(${WINE_CXX} ARGS "-v -m32 /dev/zero" OUTPUT_VARIABLE WINEBUILD_OUTPUT)
STRING(REPLACE " " ";" WINEBUILD_FLAGS "${WINEBUILD_OUTPUT}")

FOREACH(FLAG ${WINEBUILD_FLAGS})
	IF("${FLAG}" MATCHES "libwinecrt0.a.*")
		# Debian systems
		STRING(REPLACE "/lib/x86_64-" "/lib/i386-" FLAG "${FLAG}")
		# Fedora systems
		STRING(REPLACE "/lib/lib64" "/lib/i386" FLAG "${FLAG}")
		# Gentoo systems
		STRING(REPLACE "/lib/wine-" "/lib32/wine-" FLAG "${FLAG}")
		# WineHQ (/opt/wine-stable, /opt/wine-devel, /opt/wine-staging)
		STRING(REPLACE "/lib64/wine/" "/lib/wine/" FLAG "${FLAG}")

		STRING(REGEX REPLACE "/wine/libwinecrt0.a.*" "" WINE_LIBRARY_FIX "${FLAG}")
		SET(WINE_LIBRARY_FIX "${WINE_LIBRARY_FIX}/")
	ENDIF()
ENDFOREACH()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Wine DEFAULT_MSG WINE_CXX WINE_LIBRARIES WINE_INCLUDE_DIRS)

mark_as_advanced(WINE_INCLUDE_DIR WINE_LIBRARY WINE_CXX WINE_BUILD)

IF(WINE_LIBRARY_FIX)
	SET(WINE_32_FLAGS "-L${WINE_LIBRARY_FIX}wine/ -L${WINE_LIBRARY_FIX}")
ENDIF()

# Create winegcc wrapper
configure_file(${CMAKE_CURRENT_LIST_DIR}/../linux/winegcc_wrapper.in winegcc_wrapper @ONLY)
SET(WINEGCC "${CMAKE_CURRENT_BINARY_DIR}/winegcc_wrapper")
