# - Try to find PulseAudioSimple
# Once done this will define
#
#  PULSEAUDIO_FOUND - system has PulseAudioSimple
#  PULSEAUDIO_INCLUDE_DIR - the PulseAudioSimple include directory
#  PULSEAUDIO_LIBRARIES - the libraries needed to use PulseAudioSimple
#  PULSEAUDIO_DEFINITIONS - Compiler switches required for using PulseAudioSimple
#
IF(NO_PULSE)
        message(status "<disabled per request>")
ELSE(NO_PULSE)

IF (PULSEAUDIO_INCLUDE_DIR AND PULSEAUDIO_LIBRARIES)
   # in cache already
   SET(PULSEAUDIO_FIND_QUIETLY TRUE)
ENDIF (PULSEAUDIO_INCLUDE_DIR AND PULSEAUDIO_LIBRARIES)

IF (NOT WIN32)
   # use pkg-config to get the directories and then use these values
   # in the FIND_PATH() and FIND_LIBRARY() calls
   find_package(PkgConfig QUIET)
   if(PKG_CONFIG_FOUND)
      pkg_check_modules(PA libpulse)
      set(_PASIncDir ${PA_INCLUDE_DIRS})
      set(_PASLinkDir ${PA_LIBRARY_DIRS})
      set(_PASLinkFlags ${PA_LDFLAGS})
      set(_PASCflags ${PA_CFLAGS})
      set(PULSEAUDIO_DEFINITIONS ${_PASCflags})
   endif()
ENDIF (NOT WIN32)

FIND_PATH(PULSEAUDIO_INCLUDE_DIR pulse/pulseaudio.h
   PATHS
   ${_PASIncDir}
   PATH_SUFFIXES pulse
   )

FIND_LIBRARY(PULSEAUDIO_LIBRARIES NAMES pulse libpulse
   PATHS
   ${_PASLinkDir}
   )

IF (PULSEAUDIO_INCLUDE_DIR AND PULSEAUDIO_LIBRARIES)
   SET(PULSEAUDIO_FOUND TRUE)
ELSE (PULSEAUDIO_INCLUDE_DIR AND PULSEAUDIO_LIBRARIES)
   SET(PULSEAUDIO_FOUND FALSE)
ENDIF (PULSEAUDIO_INCLUDE_DIR AND PULSEAUDIO_LIBRARIES)

IF (PULSEAUDIO_FOUND)
   IF (NOT PULSEAUDIO_FIND_QUIETLY)
      MESSAGE(STATUS "Found PulseAudio Simple: ${PULSEAUDIO_LIBRARIES}")
   ENDIF (NOT PULSEAUDIO_FIND_QUIETLY)
   SET(USE_PULSE_ 1)
ELSE (PULSEAUDIO_FOUND)
      MESSAGE(STATUS "Could NOT find LibXml2")
ENDIF (PULSEAUDIO_FOUND)

MARK_AS_ADVANCED(PULSEAUDIO_INCLUDE_DIR PULSEAUDIO_LIBRARIES)

ENDIf(NO_PULSE)

