# Finds the Aubio library.
# Copyright (c) 2022, saker <sakertooth@gmail.com>

# AUBIO_FOUND - aubio is available on the system
# AUBIO_INCLUDE_DIRS - the include directory for aubio
# AUBIO_LIBRARY - the aubio library

# Distributed under the MIT License. http://opensource.org/licenses/MIT

find_package(PkgConfig)
pkg_check_modules(PC_aubio QUIET aubio)

find_path(AUBIO_INCLUDE_DIR
  NAMES aubio.h
  PATHS ${PC_aubio_INCLUDE_DIRS}
  PATH_SUFFIXES aubio
)

find_library(AUBIO_LIBRARY
  NAMES aubio
  PATHS ${PC_aubio_LIBRARY_DIRS}
)

if(PKG_CONFIG_FOUND)
  set(AUBIO_VERSION ${PC_aubio_VERSION})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Aubio
  FOUND_VAR AUBIO_FOUND
  REQUIRED_VARS
    AUBIO_LIBRARY
    AUBIO_INCLUDE_DIR
  VERSION_VAR AUBIO_VERSION
)

mark_as_advanced(
  AUBIO_INCLUDE_DIR
  AUBIO_LIBRARY
)