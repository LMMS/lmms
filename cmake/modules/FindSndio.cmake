# sndio check, based on FindAlsa.cmake
#

# Copyright (c) 2006, David Faure, <faure@kde.org>
# Copyright (c) 2007, Matthias Kretz <kretz@kde.org>
# Copyright (c) 2009, Jacob Meuser <jakemsr@sdf.lonestar.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(CheckIncludeFiles)
include(CheckIncludeFileCXX)
include(CheckLibraryExists)

# Already done by toplevel
find_library(SNDIO_LIBRARY sndio)
set(SNDIO_LIBRARY_DIR "")
if(SNDIO_LIBRARY)
   get_filename_component(SNDIO_LIBRARY_DIR ${SNDIO_LIBRARY} PATH)
endif(SNDIO_LIBRARY)

check_library_exists(sndio sio_open "${SNDIO_LIBRARY_DIR}" HAVE_SIO_OPEN)

find_path(SNDIO_INCLUDE_DIR sndio.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SNDIO DEFAULT_MSG SNDIO_LIBRARY SNDIO_INCLUDE_DIR HAVE_SIO_OPEN)

if(SNDIO_FOUND)
    set(SNDIO_INCLUDE_DIRS "${SNDIO_INCLUDE_DIR}")
    set(SNDIO_LIBRARIES "${SNDIO_LIBRARY}")
endif(HAVE_SNDIO)

mark_as_advanced(SNDIO_INCLUDE_DIR SNDIO_LIBRARY SNDIO_INCLUDE_DIRS SNDIO_LIBRARIES)
