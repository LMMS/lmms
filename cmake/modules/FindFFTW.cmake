# FindFFTW.cmake - Try to find FFTW3
# Copyright (c) 2018 Lukas W <lukaswhl/at/gmail.com>
# This file is MIT licensed.
# See http://opensource.org/licenses/MIT

# Try pkgconfig for hints
find_package(PkgConfig QUIET)

if(NOT FFTW_FIND_COMPONENTS)
	set(FFTW_FIND_COMPONENTS fftw3f fftw3 fftwl)
endif()

if(PKG_CONFIG_FOUND)
	pkg_check_modules(FFTW_PKG QUIET fftw>=3.0.0)
	pkg_check_modules(FFTW3_PKG QUIET fftw3>=3.0.0)
endif()

find_path(FFTW_INCLUDE_DIR
	NAMES fftw3.h
	PATHS ${FFTW_PKG_INCLUDE_DIRS} ${FFTW3_PKG_INCLUDE_DIRS}
)

set(check_list)

foreach(comp ${FFTW_FIND_COMPONENTS})
	string(TOUPPER ${comp} COMP)
	if(PKG_CONFIG_FOUND)
		pkg_check_modules(${COMP}_PKG QUIET ${comp}>=3.0.0)
	endif()

	find_library(${COMP}_LIBRARY
		NAMES ${comp}
		PATHS ${comp}_PKG_LIBRARY_DIRS
 	)
 	list(APPEND check_list ${COMP}_LIBRARY)

	set(${COMP}_LIBRARIES ${${COMP}_LIBRARY} CACHE FILEPATH "${COMP} library path")
	set(${COMP}_INCLUDE_DIRS ${FFTW_INCLUDE_DIR} CACHE PATH "${COMP} include path")
	mark_as_advanced(${COMP}_LIBRARIES ${COMP}_INCLUDE_DIRS)
endforeach()

find_package(PackageHandleStandardArgs)	
find_package_handle_standard_args(FFTW DEFAULT_MSG FFTW_INCLUDE_DIR ${check_list})

set(FFTW_INCLUDE_DIRS ${FFTW_INCLUDE_DIR})

mark_as_advanced(FFTW_LIBRARY FFTW_LIBRARIES FFTW_INCLUDE_DIR FFTW_INCLUDE_DIRS ${check_list})
