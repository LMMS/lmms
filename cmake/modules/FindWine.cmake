# - Try to find the wine libraries
# Once done this will define
#
#  WINE_FOUND - System has wine
#  WINE_INCLUDE_DIRS - The wine include directories
#  WINE_LIBRARIES - The libraries needed to use wine
#  WINE_DEFINITIONS - Compiler switches required for using wine
#

LIST(APPEND CMAKE_PREFIX_PATH /opt/wine-stable /opt/wine-devel /opt/wine-staging)

FIND_PATH(WINE_INCLUDE_DIR windows/windows.h PATH_SUFFIXES wine)
FIND_LIBRARY(WINE_LIBRARY NAMES wine PATH_SUFFIXES wine i386-linux-gnu/wine)
FIND_PROGRAM(WINE_CXX NAMES wineg++ winegcc winegcc64 winegcc32)

set(WINE_INCLUDE_DIRS ${WINE_INCLUDE_DIR} )
set(WINE_LIBRARIES ${WINE_LIBRARY} )

# Handle wine linking problems
EXEC_PROGRAM(${WINE_CXX} ARGS "-v -m32 /dev/zero" OUTPUT_VARIABLE WINEBUILD_OUTPUT)

# Debian systems 
IF("${WINEBUILD_OUTPUT}" MATCHES ".*x86_64-linux-gnu/wine/libwinecrt0.a.*")
	SET(WINE_LIBRARY_FIX "/usr/lib/i386-linux-gnu/wine/" )
# Fedora systems 
ELSEIF("${WINEBUILD_OUTPUT}" MATCHES ".*lib64/wine/libwinecrt0.a.*")
	SET(WINE_LIBRARY_FIX "/usr/lib/i386/wine/")
# Wine stable
ELSEIF("${WINEBUILD_OUTPUT}" MATCHES "/opt/wine-stable/lib64/wine/libwinecrt0.a.*")
	SET(WINE_LIBRARY_FIX "/opt/wine-stable/lib/wine/")
# Wine development
ELSEIF("${WINEBUILD_OUTPUT}" MATCHES "/opt/wine-devel/lib64/wine/libwinecrt0.a.*")
	SET(WINE_LIBRARY_FIX "/opt/wine-devel/lib/wine/")
# Wine staging
ELSEIF("${WINEBUILD_OUTPUT}" MATCHES "/opt/wine-staging/lib64/wine/libwinecrt0.a.*")
	SET(WINE_LIBRARY_FIX "/opt/wine-staging/lib/wine/")
ENDIF()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Wine DEFAULT_MSG WINE_LIBRARIES WINE_INCLUDE_DIRS)

mark_as_advanced(WINE_INCLUDE_DIR WINE_LIBRARY)
