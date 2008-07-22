# - Try to find the OggVorbis libraries
# Once done this will define
#
#  OGGVORBIS_FOUND - system has OggVorbis
#  OGGVORBIS_VERSION - set either to 1 or 2
#  OGGVORBIS_INCLUDE_DIR - the OggVorbis include directory
#  OGGVORBIS_LIBRARIES - The libraries needed to use OggVorbis
#  OGG_LIBRARY         - The Ogg library
#  VORBIS_LIBRARY      - The Vorbis library
#  VORBISFILE_LIBRARY  - The VorbisFile library
#  VORBISENC_LIBRARY   - The VorbisEnc library

# Copyright (c) 2006, Richard Laerkaeng, <richard@goteborg.utfors.se>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


include (CheckLibraryExists)

find_path(VORBIS_INCLUDE_DIR vorbis/vorbisfile.h)
find_path(OGG_INCLUDE_DIR ogg/ogg.h)

find_library(OGG_LIBRARY NAMES ogg)
find_library(VORBIS_LIBRARY NAMES vorbis)
find_library(VORBISFILE_LIBRARY NAMES vorbisfile)
find_library(VORBISENC_LIBRARY NAMES vorbisenc)


if (VORBIS_INCLUDE_DIR AND VORBIS_LIBRARY AND VORBISFILE_LIBRARY AND VORBISENC_LIBRARY)
   set(OGGVORBIS_FOUND TRUE)

   set(OGGVORBIS_LIBRARIES ${OGG_LIBRARY} ${VORBIS_LIBRARY} ${VORBISFILE_LIBRARY} ${VORBISENC_LIBRARY})

   set(_CMAKE_REQUIRED_LIBRARIES_TMP ${CMAKE_REQUIRED_LIBRARIES})
   set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${OGGVORBIS_LIBRARIES})
   check_library_exists(vorbis vorbis_bitrate_addblock "" HAVE_LIBVORBISENC2)
   set(CMAKE_REQUIRED_LIBRARIES ${_CMAKE_REQUIRED_LIBRARIES_TMP})

   if (HAVE_LIBVORBISENC2)
      set (OGGVORBIS_VERSION 2)
   else (HAVE_LIBVORBISENC2)
      set (OGGVORBIS_VERSION 1)
   endif (HAVE_LIBVORBISENC2)

else (VORBIS_INCLUDE_DIR AND VORBIS_LIBRARY AND VORBISFILE_LIBRARY AND VORBISENC_LIBRARY)
   set (OGGVORBIS_VERSION)
   set(OGGVORBIS_FOUND FALSE)
endif (VORBIS_INCLUDE_DIR AND VORBIS_LIBRARY AND VORBISFILE_LIBRARY AND VORBISENC_LIBRARY)


if (OGGVORBIS_FOUND)
   if (NOT OggVorbis_FIND_QUIETLY)
      message(STATUS "Found OggVorbis: ${OGGVORBIS_LIBRARIES}")
   endif (NOT OggVorbis_FIND_QUIETLY)
else (OGGVORBIS_FOUND)
   if (OggVorbis_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find OggVorbis libraries")
   endif (OggVorbis_FIND_REQUIRED)
   if (NOT OggVorbis_FIND_QUITELY)
      message(STATUS "Could NOT find OggVorbis libraries")
   endif (NOT OggVorbis_FIND_QUITELY)
endif (OGGVORBIS_FOUND)

#check_include_files(vorbis/vorbisfile.h HAVE_VORBISFILE_H)
#check_library_exists(ogg ogg_page_version "" HAVE_LIBOGG)
#check_library_exists(vorbis vorbis_info_init "" HAVE_LIBVORBIS)
#check_library_exists(vorbisfile ov_open "" HAVE_LIBVORBISFILE)
#check_library_exists(vorbisenc vorbis_info_clear "" HAVE_LIBVORBISENC)
#check_library_exists(vorbis vorbis_bitrate_addblock "" HAVE_LIBVORBISENC2)

#if (HAVE_LIBOGG AND HAVE_VORBISFILE_H AND HAVE_LIBVORBIS AND HAVE_LIBVORBISFILE AND HAVE_LIBVORBISENC)
#    message(STATUS "Ogg/Vorbis found")
#    set (VORBIS_LIBS "-lvorbis -logg")
#    set (VORBISFILE_LIBS "-lvorbisfile")
#    set (VORBISENC_LIBS "-lvorbisenc")
#    set (OGGVORBIS_FOUND TRUE)
#    if (HAVE_LIBVORBISENC2)
#        set (HAVE_VORBIS 2)
#    else (HAVE_LIBVORBISENC2)
#        set (HAVE_VORBIS 1)
#    endif (HAVE_LIBVORBISENC2)
#else (HAVE_LIBOGG AND HAVE_VORBISFILE_H AND HAVE_LIBVORBIS AND HAVE_LIBVORBISFILE AND HAVE_LIBVORBISENC)
#    message(STATUS "Ogg/Vorbis not found")
#endif (HAVE_LIBOGG AND HAVE_VORBISFILE_H AND HAVE_LIBVORBIS AND HAVE_LIBVORBISFILE AND HAVE_LIBVORBISENC)

