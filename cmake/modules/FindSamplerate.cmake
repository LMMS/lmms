# FindFFTW.cmake - Try to find FFTW3
# Copyright (c) 2018 Lukas W <lukaswhl/at/gmail.com>
# This file is MIT licensed.
# See http://opensource.org/licenses/MIT

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
	pkg_check_modules(SAMPLERATE_PKG samplerate)
endif()

find_path(SAMPLERATE_INCLUDE_DIR
	NAMES samplerate.h
	PATHS ${SAMPLERATE_PKG_INCLUDE_DIRS}
)

set(SAMPLERATE_NAMES samplerate libsamplerate)
if(Samplerate_FIND_VERSION_MAJOR)
	list(APPEND SAMPLERATE_NAMES libsamplerate-${Samplerate_FIND_VERSION_MAJOR})
else()
	list(APPEND SAMPLERATE_NAMES libsamplerate-0)
endif()

find_library(SAMPLERATE_LIBRARY
	NAMES ${SAMPLERATE_NAMES}
	PATHS ${SAMPLERATE_PKG_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SAMPLERATE DEFAULT_MSG SAMPLERATE_LIBRARY SAMPLERATE_INCLUDE_DIR)

mark_as_advanced(SAMPLERATE_INCLUDE_DIR SAMPLERATE_LIBRARY )

set(SAMPLERATE_LIBRARIES ${SAMPLERATE_LIBRARY} )
set(SAMPLERATE_INCLUDE_DIRS ${SAMPLERATE_INCLUDE_DIR})
