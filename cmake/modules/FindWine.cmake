# - Try to find the wine libraries
# Once done this will define
#
#  WINE_FOUND - System has wine
#  WINE_INCLUDE_DIRS - The wine include directories
#  WINE_LIBRARIES - The libraries needed to use wine
#  WINE_DEFINITIONS - Compiler switches required for using wine
#

FIND_PATH(WINE_INCLUDE_DIR windows/windows.h PATH_SUFFIXES wine)
FIND_LIBRARY(WINE_LIBRARY NAMES wine PATH_SUFFIXES wine)
FIND_PROGRAM(WINE_CXX NAMES wineg++)

set(WINE_INCLUDE_DIRS ${WINE_INCLUDE_DIR} )
set(WINE_LIBRARIES ${WINE_LIBRARY} )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Wine DEFAULT_MSG WINE_LIBRARIES WINE_INCLUDE_DIRS)

mark_as_advanced(WINE_INCLUDE_DIR WINE_LIBRARY)
