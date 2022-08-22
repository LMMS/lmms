# Distributed under the MIT License.
# Copyright (c) 2022 sakertooth <sakertooth@gmail.com>

#[=======================================================================[.rst:
FindFFMPEG
-------

Finds the FFmpeg library.

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``FFMPEG_FOUND``
  True if the system has found the required FFmpeg components.

``FFMPEG_INCLUDE_DIRS``
  The include directories of the required FFmpeg components.

``FFMPEG_LIBRARIES``
  The libraries of the required FFmpeg components.


Cache Variables
^^^^^^^^^^^^^^^

For each FFmpeg component:
  avcodec,
  avformat,
  avutil,
  avfilter,
  avdevice,
  swresample,
  swscale

The following cache variables may also be set:

``FFMPEG_<component>_INCLUDE_DIR``
  The include directory for the FFmpeg component.

``FFMPEG_<component>_LIBRARY``
  The library for the FFmpeg component.

#]=======================================================================]

find_package(PkgConfig QUIET)
set(_possibleComponents avcodec avformat avutil avfilter avdevice swresample swscale)

foreach(component ${FFMPEG_FIND_COMPONENTS})
  string(TOUPPER ${component} _upperComponent)
  pkg_check_modules(PC_${_upperComponent} lib${component})

  LIST(FIND _possibleComponents ${component} _componentIndex)
  if(${_componentIndex} EQUAL -1)
    message(FATAL_ERROR "Unknown FFmpeg component: ${component}")
  endif()

  find_path(FFMPEG_${_upperComponent}_INCLUDE_DIR
    NAMES lib${component}/${component}.h
    PATHS ${PC_${_upperComponent}_INCLUDE_DIRS}
  )

  find_library(FFMPEG_${_upperComponent}_LIBRARY
    NAMES ${component}
    PATHS ${PC_${_upperComponent}_LIBRARY_DIR}
  )

  if (FFMPEG_${_upperComponent}_INCLUDE_DIR AND FFMPEG_${_upperComponent}_LIBRARY)
    set(FFMPEG_${_upperComponent}_FOUND TRUE)
    set(FFMPEG_${_upperComponent}_VERSION ${PC_${_upperComponent}_VERSION})
    set(FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIRS} ${FFMPEG_${_upperComponent}_INCLUDE_DIR})
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} ${FFMPEG_${_upperComponent}_LIBRARY})

    mark_as_advanced(
      FFMPEG_${_upperComponent}_INCLUDE_DIR
      FFMPEG_${_upperComponent}_LIBRARY
    )
  endif()

  set(FFMPEG_REQUIRED_VARS ${FFMPEG_REQUIRED_VARS}
      FFMPEG_${_upperComponent}_INCLUDE_DIR
      FFMPEG_${_upperComponent}_LIBRARY
  )
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFMPEG DEFAULT_MSG ${FFMPEG_REQUIRED_VARS})