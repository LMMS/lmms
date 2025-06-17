# - Try to find the wine libraries
# Once done this will define
#
#  WINE_FOUND - System has wine
#
#  WINE_INCLUDE_DIR - Wine include directory
#  WINE_BUILD - Path to winebuild
#  WINE_CXX - Path to wineg++
#  WINE_GCC - Path to winegcc
#  WINE_32_LIBRARY_DIRS - Path(s) to 32-bit wine libs
#  WINE_32_FLAGS - 32-bit linker flags
#  WINE_64_LIBRARY_DIRS - Path(s) to 64-bit wine libs
#  WINE_64_FLAGS - 64-bit linker flags
#

MACRO(_findwine_find_flags output expression result)
	STRING(REPLACE " " ";" WINEBUILD_FLAGS "${output}")
	FOREACH(FLAG ${WINEBUILD_FLAGS})
		IF("${FLAG}" MATCHES "${expression}")
			LIST(APPEND ${result} "${FLAG}")
		ENDIF()
	ENDFOREACH()
ENDMACRO()

MACRO(_regex_replace_foreach EXPRESSION REPLACEMENT RESULT INPUT)
	SET(${RESULT} "")
	FOREACH(ITEM ${INPUT})
		STRING(REGEX REPLACE "${EXPRESSION}" "${REPLACEMENT}" ITEM "${ITEM}")
		LIST(APPEND ${RESULT} "${ITEM}")
	ENDFOREACH()
ENDMACRO()

# Prefer newest wine first
list(APPEND WINE_LOCATIONS
	/opt/wine-staging
	/opt/wine-devel
	/opt/wine-stable
	# Gentoo Systems
	/etc/eselect/wine
	/usr/lib/wine)

# Prepare bin search
foreach(_loc ${WINE_LOCATIONS})
	if(_loc STREQUAL /usr/lib/wine)
		# /usr/lib/wine doesn't have a "bin"
		list(APPEND WINE_CXX_LOCATIONS "${_loc}")
	else()
		# expect "bin"
		list(APPEND WINE_CXX_LOCATIONS "${_loc}/bin")
	endif()
endforeach()
# Fallback
list(APPEND WINE_CXX_LOCATIONS "/usr/bin")

# Prefer most-common to least common
FIND_PROGRAM(WINE_CXX NAMES
		wineg++
		wineg++-stable
	PATHS
		${WINE_CXX_LOCATIONS}
	NO_DEFAULT_PATH
)

FIND_PROGRAM(WINE_GCC NAMES
		winegcc
		winegcc-stable
	PATHS
		${WINE_CXX_LOCATIONS}
	NO_DEFAULT_PATH
)

FIND_PROGRAM(WINE_BUILD NAMES winebuild PATHS ${WINE_CXX_LOCATIONS} NO_DEFAULT_PATH)
# Detect wine paths and handle linking problems
IF(WINE_CXX)
	# call wineg++ to obtain implied includes and libs
	execute_process(COMMAND ${WINE_CXX} -m32 -v /dev/zero OUTPUT_VARIABLE WINEBUILD_OUTPUT_32 ERROR_QUIET)
	execute_process(COMMAND ${WINE_CXX} -m64 -v /dev/zero OUTPUT_VARIABLE WINEBUILD_OUTPUT_64 ERROR_QUIET)
	_findwine_find_flags("${WINEBUILD_OUTPUT_32}" "^-isystem/usr/include$" BUGGED_WINEGCC)
	_findwine_find_flags("${WINEBUILD_OUTPUT_32}" "^-isystem" WINEGCC_INCLUDE_DIR)
	_findwine_find_flags("${WINEBUILD_OUTPUT_32}" "libwinecrt0\\.a.*" WINECRT_32)
	_findwine_find_flags("${WINEBUILD_OUTPUT_64}" "libwinecrt0\\.a.*" WINECRT_64)
	_regex_replace_foreach("^-isystem" "" WINE_INCLUDE_HINT "${WINEGCC_INCLUDE_DIR}")
	_regex_replace_foreach("/wine/windows$" "" WINE_INCLUDE_HINT "${WINE_INCLUDE_HINT}")
	STRING(REGEX REPLACE "wine/libwinecrt0\\.a.*" "" WINE_32_LIBRARY_DIR "${WINECRT_32}")
	STRING(REGEX REPLACE "wine/libwinecrt0\\.a.*" "" WINE_64_LIBRARY_DIR "${WINECRT_64}")
	# Handle winehq
	STRING(REGEX REPLACE "/libwinecrt0\\.a.*" "/" WINE_32_LIBRARY_DIR "${WINE_32_LIBRARY_DIR}")
	STRING(REGEX REPLACE "/libwinecrt0\\.a.*" "/" WINE_64_LIBRARY_DIR "${WINE_64_LIBRARY_DIR}")

	IF(BUGGED_WINEGCC)
		MESSAGE(WARNING "Your winegcc is unusable due to https://bugs.winehq.org/show_bug.cgi?id=46293,\n
			Consider either upgrading or downgrading wine.")
		RETURN()
	ENDIF()
	IF(WINE_32_LIBRARY_DIR STREQUAL WINE_64_LIBRARY_DIR)
		MESSAGE(STATUS "Old winegcc detected, trying to use workaround linking")
		# Fix library search directory according to the target bitness
		IF(WINE_32_LIBRARY_DIR MATCHES "/lib/(x86_64|i386)-")
			# Debian systems
			STRING(REPLACE "/lib/x86_64-" "/lib/i386-" WINE_32_LIBRARY_DIR "${WINE_32_LIBRARY_DIR}")
			STRING(REPLACE "/lib/i386-" "/lib/x86_64-" WINE_64_LIBRARY_DIR "${WINE_64_LIBRARY_DIR}")
		ELSEIF(WINE_32_LIBRARY_DIR MATCHES "/(lib|lib64)/wine/$")
			# WineHQ (/opt/wine-stable, /opt/wine-devel, /opt/wine-staging)
			STRING(REGEX REPLACE "/lib64/wine/$" "/lib/wine/" WINE_32_LIBRARY_DIR "${WINE_32_LIBRARY_DIR}")
			STRING(REGEX REPLACE "/lib/wine/$" "/lib64/wine/" WINE_64_LIBRARY_DIR "${WINE_64_LIBRARY_DIR}")
		ELSEIF(WINE_32_LIBRARY_DIR MATCHES "/lib32/")
			# Systems with old multilib layout
			STRING(REPLACE "/lib32/" "/lib/" WINE_64_LIBRARY_DIR "${WINE_32_LIBRARY_DIR}")
		ELSEIF(WINE_32_LIBRARY_DIR MATCHES "/lib64/")
			# We need to test if the corresponding 64bit library directory is lib or lib32
			STRING(REPLACE "/lib64/" "/lib32/" WINE_32_LIBRARY_DIR "${WINE_64_LIBRARY_DIR}")
			IF(NOT EXISTS "${WINE_32_LIBRARY_DIR}")
				STRING(REPLACE "/lib64/" "/lib/" WINE_32_LIBRARY_DIR "${WINE_64_LIBRARY_DIR}")
			ENDIF()
		ELSEIF(WINE_32_LIBRARY_DIR MATCHES "/lib/")
			# Test if this directory is for 32bit or 64bit
			STRING(REPLACE "/lib/" "/lib32/" WINE_32_LIBRARY_DIR "${WINE_64_LIBRARY_DIR}")
			IF(NOT EXISTS "${WINE_32_LIBRARY_DIR}")
				SET(WINE_32_LIBRARY_DIR "${WINE_64_LIBRARY_DIR}")
				STRING(REPLACE "/lib/" "/lib64/" WINE_64_LIBRARY_DIR "${WINE_64_LIBRARY_DIR}")
			ENDIF()
		ELSE()
			MESSAGE(WARNING "Can't detect wine installation layout. You may get some build errors.")
		ENDIF()
		SET(WINE_LIBRARY_FIX "${WINE_32_LIBRARY_DIR} and ${WINE_64_LIBRARY_DIR}")
	ENDIF()
ENDIF()

FIND_PATH(WINE_INCLUDE_DIR wine/exception.h
	HINTS ${WINE_INCLUDE_HINT}
)

SET(_ARCHITECTURE ${CMAKE_LIBRARY_ARCHITECTURE})

SET(CMAKE_LIBRARY_ARCHITECTURE ${_ARCHITECTURE})

SET(WINE_INCLUDE_DIRS ${WINE_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Wine DEFAULT_MSG WINE_CXX WINE_INCLUDE_DIRS)

mark_as_advanced(WINE_INCLUDE_DIR WINE_LIBRARY WINE_CXX WINE_BUILD)

IF(WINE_32_LIBRARY_DIR)
	IF(WINE_32_LIBRARY_DIR MATCHES "^/opt/wine-.*")
		# winehq uses a singular lib directory
		SET(WINE_32_FLAGS "-L${WINE_32_LIBRARY_DIR}")
		SET(WINE_32_LIBRARY_DIRS "${WINE_32_LIBRARY_DIR}")
	ELSEIF(WINE_32_LIBRARY_DIR MATCHES "wine*/lib")
		SET(WINE_32_FLAGS "-L${WINE_32_LIBRARY_DIR} -L${WINE_32_LIBRARY_DIR}../")
		SET(WINE_32_LIBRARY_DIRS "${WINE_32_LIBRARY_DIR}:${WINE_32_LIBRARY_DIR}/..")
	ELSE()
		SET(WINE_32_FLAGS "-L${WINE_32_LIBRARY_DIR} -L${WINE_32_LIBRARY_DIR}wine/")
		SET(WINE_32_LIBRARY_DIRS "${WINE_32_LIBRARY_DIR}:${WINE_32_LIBRARY_DIR}wine/")
	ENDIF()
ENDIF()

IF(WINE_64_LIBRARY_DIR)
	IF(WINE_32_LIBRARY_DIR MATCHES "^/opt/wine-.*")
		# winehq uses a singular lib directory
		SET(WINE_64_FLAGS "-L${WINE_64_LIBRARY_DIR}")
		SET(WINE_64_LIBRARY_DIRS "${WINE_64_LIBRARY_DIR}")
	ELSEIF(WINE_64_LIBRARY_DIR MATCHES "wine*/lib")
		SET(WINE_64_FLAGS "-L${WINE_64_LIBRARY_DIR} -L${WINE_64_LIBRARY_DIR}../")
		SET(WINE_64_LIBRARY_DIRS "${WINE_64_LIBRARY_DIR}:${WINE_64_LIBRARY_DIR}/..")
	ELSE()
		SET(WINE_64_FLAGS "-L${WINE_64_LIBRARY_DIR} -L${WINE_64_LIBRARY_DIR}wine/")
		SET(WINE_64_LIBRARY_DIRS "${WINE_64_LIBRARY_DIR}:${WINE_64_LIBRARY_DIR}wine/")
	ENDIF()
ENDIF()

message(STATUS "  WINE_INCLUDE_DIR:     ${WINE_INCLUDE_DIR}")
message(STATUS "  WINE_CXX:             ${WINE_CXX}")
message(STATUS "  WINE_GCC:             ${WINE_GCC}")
message(STATUS "  WINE_32_FLAGS:        ${WINE_32_FLAGS}")
message(STATUS "  WINE_64_FLAGS:        ${WINE_64_FLAGS}")

# Create winegcc (technically, wineg++) wrapper
configure_file(${CMAKE_CURRENT_LIST_DIR}/winegcc_wrapper.in winegcc_wrapper @ONLY)
SET(WINEGCC "${CMAKE_CURRENT_BINARY_DIR}/winegcc_wrapper")
