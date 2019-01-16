# FindJack.cmake
# This file is MIT licensed.
# See http://opensource.org/licenses/MIT
#
# This package will define
# JACK_FOUND
# JACK_LIBRARIES
# JACK_INCLUDE_DIRS

# Try pkgconfig
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
	pkg_check_modules(JACK_PKG jack>=0.77)
endif(PKG_CONFIG_FOUND)

find_path(JACK_INCLUDE_DIR 
	NAMES jack/jack.h
	PATHS JACK_PKG_INCLUDE_DIRS)
find_library(JACK_LIBRARY
	NAMES jack 
	PATHS JACK_PKG_LIBRARY_DIRS)

find_package(PackageHandleStandardArgs)
find_package_handle_standard_args(Jack DEFAULT_MSG JACK_LIBRARY JACK_INCLUDE_DIR)

set(JACK_LIBRARIES ${JACK_LIBRARY})
set(JACK_INCLUDE_DIRS ${JACK_INCLUDE_DIR})

mark_as_advanced(JACK_LIBRARY JACK_INCLUDE_DIR)
