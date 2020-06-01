# FindSndFile.cmake - Try to find libsndfile
# Copyright (c) 2018 Lukas W <lukaswhl/at/gmail.com>
# This file is MIT licensed.
# See http://opensource.org/licenses/MIT

# Try pkgconfig for hints
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
	pkg_check_modules(SNDFILE_PKG sndfile)
endif(PKG_CONFIG_FOUND)
set(SndFile_DEFINITIONS ${SNDFILE_PKG_CFLAGS_OTHER})

if(WIN32)
	# Try Vcpkg
	find_package(LibSndFile ${SndFile_FIND_VERSION} CONFIG QUIET)
	if(LibSndFile_FOUND)
		get_target_property(LibSndFile_Location sndfile-shared LOCATION)
		get_target_property(LibSndFile_Include_Path sndfile-shared INTERFACE_INCLUDE_DIRECTORIES)
		get_filename_component(LibSndFile_Path LibSndFile_Location PATH)
	endif()
endif()

find_path(SNDFILE_INCLUDE_DIR
	NAMES sndfile.h
	PATHS ${SNDFILE_PKG_INCLUDE_DIRS} ${LibSndFile_Include_Path}
)

find_library(SNDFILE_LIBRARY
	NAMES sndfile libsndfile libsndfile-1
	PATHS ${SNDFILE_PKG_LIBRARY_DIRS} ${LibSndFile_Path}
)

find_package(PackageHandleStandardArgs)
find_package_handle_standard_args(SndFile DEFAULT_MSG SNDFILE_LIBRARY SNDFILE_INCLUDE_DIR)

set(SNDFILE_LIBRARIES ${SNDFILE_LIBRARY})
set(SNDFILE_INCLUDE_DIRS ${SNDFILE_INCLUDE_DIR})

mark_as_advanced(SNDFILE_LIBRARY SNDFILE_LIBRARIES SNDFILE_INCLUDE_DIR SNDFILE_INCLUDE_DIRS)
