# - Try to find LAME
# Once done this will define
#
# LAME_FOUND - system has liblame
# LAME_INCLUDE_DIRS - the liblame include directory
# LAME_LIBRARIES - The liblame libraries

find_path(LAME_INCLUDE_DIRS lame/lame.h)
find_library(LAME_LIBRARIES mp3lame)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Lame DEFAULT_MSG LAME_INCLUDE_DIRS LAME_LIBRARIES)

list(APPEND LAME_DEFINITIONS -DHAVE_LIBMP3LAME=1)

mark_as_advanced(LAME_INCLUDE_DIRS LAME_LIBRARIES LAME_DEFINITIONS)
