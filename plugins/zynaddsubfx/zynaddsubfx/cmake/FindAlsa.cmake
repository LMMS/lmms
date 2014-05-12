# Alsa check, based on libkmid/configure.in.in.
# Only the support for Alsa >= 0.9.x was included; 0.5.x was dropped (but feel free to re-add it if you need it)
# It defines ...
# It offers the following macros:
#  ALSA_CONFIGURE_FILE(config_header) - generate a config.h, typical usage: 
#                                       ALSA_CONFIGURE_FILE(${CMAKE_BINARY_DIR}/config-alsa.h)
#  ALSA_VERSION_STRING(version_string)  looks for alsa/version.h and reads the version string into
#                                       the first argument passed to the macro

# Copyright (c) 2006, David Faure, <faure@kde.org>
# Copyright (c) 2007, Matthias Kretz <kretz@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(CheckIncludeFiles)
include(CheckIncludeFileCXX)
include(CheckLibraryExists)

# Already done by toplevel
find_library(ASOUND_LIBRARY asound)
set(ASOUND_LIBRARY_DIR "")
if(ASOUND_LIBRARY)
   get_filename_component(ASOUND_LIBRARY_DIR ${ASOUND_LIBRARY} PATH)
endif(ASOUND_LIBRARY)

check_library_exists(asound snd_seq_create_simple_port "${ASOUND_LIBRARY_DIR}" HAVE_LIBASOUND2)
if(HAVE_LIBASOUND2)
    message(STATUS "Found ALSA: ${ASOUND_LIBRARY}")
else(HAVE_LIBASOUND2)
    message(STATUS "ALSA not found")
endif(HAVE_LIBASOUND2)
set(ALSA_FOUND ${HAVE_LIBASOUND2})

find_path(ALSA_INCLUDES alsa/version.h)

macro(ALSA_VERSION_STRING _result)
    # check for version in alsa/version.h
    if(ALSA_INCLUDES)
        file(READ "${ALSA_INCLUDES}/alsa/version.h" _ALSA_VERSION_CONTENT)
        string(REGEX REPLACE ".*SND_LIB_VERSION_STR.*\"(.*)\".*" "\\1" ${_result} ${_ALSA_VERSION_CONTENT})
    else(ALSA_INCLUDES)
        message(STATUS "ALSA version not known. ALSA output will probably not work correctly.")
    endif(ALSA_INCLUDES)
endmacro(ALSA_VERSION_STRING _result)


get_filename_component(_FIND_ALSA_MODULE_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)
macro(ALSA_CONFIGURE_FILE _destFile)
    check_include_files(sys/soundcard.h HAVE_SYS_SOUNDCARD_H)
    check_include_files(machine/soundcard.h HAVE_MACHINE_SOUNDCARD_H)

    check_include_files(linux/awe_voice.h HAVE_LINUX_AWE_VOICE_H)
    check_include_files(awe_voice.h HAVE_AWE_VOICE_H)
    check_include_files(/usr/src/sys/i386/isa/sound/awe_voice.h HAVE__USR_SRC_SYS_I386_ISA_SOUND_AWE_VOICE_H)
    check_include_files(/usr/src/sys/gnu/i386/isa/sound/awe_voice.h HAVE__USR_SRC_SYS_GNU_I386_ISA_SOUND_AWE_VOICE_H)

    check_include_file_cxx(sys/asoundlib.h HAVE_SYS_ASOUNDLIB_H)
    check_include_file_cxx(alsa/asoundlib.h HAVE_ALSA_ASOUNDLIB_H)

    check_library_exists(asound snd_pcm_resume "${ASOUND_LIBRARY_DIR}" ASOUND_HAS_SND_PCM_RESUME)
    if(ASOUND_HAS_SND_PCM_RESUME)
        set(HAVE_SND_PCM_RESUME 1)
    endif(ASOUND_HAS_SND_PCM_RESUME)

    configure_file(${_FIND_ALSA_MODULE_DIR}/config-alsa.h.cmake ${_destFile})
endmacro(ALSA_CONFIGURE_FILE _destFile)

mark_as_advanced(ALSA_INCLUDES ASOUND_LIBRARY)
