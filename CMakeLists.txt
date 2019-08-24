CMAKE_MINIMUM_REQUIRED(VERSION 3.3)

PROJECT(lmms)

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})
SET(LMMS_BINARY_DIR ${CMAKE_BINARY_DIR})
SET(LMMS_SOURCE_DIR ${CMAKE_SOURCE_DIR})

IF(COMMAND CMAKE_POLICY)
	CMAKE_POLICY(SET CMP0005 NEW)
	CMAKE_POLICY(SET CMP0003 NEW)
	IF (CMAKE_MAJOR_VERSION GREATER 2)
		CMAKE_POLICY(SET CMP0026 NEW)
		CMAKE_POLICY(SET CMP0045 NEW)
		CMAKE_POLICY(SET CMP0050 OLD)
	ENDIF()
	CMAKE_POLICY(SET CMP0020 NEW)
	CMAKE_POLICY(SET CMP0057 NEW)
ENDIF(COMMAND CMAKE_POLICY)

INCLUDE(CheckSubmodules)
INCLUDE(AddFileDependencies)
INCLUDE(CheckIncludeFiles)
INCLUDE(FindPkgConfig)
INCLUDE(GenerateExportHeader)

STRING(TOUPPER          "${CMAKE_PROJECT_NAME}" PROJECT_NAME_UCASE)

SET(PROJECT_YEAR 2019)

SET(PROJECT_AUTHOR      "LMMS Developers")
SET(PROJECT_URL         "https://lmms.io")
SET(PROJECT_EMAIL       "lmms-devel@lists.sourceforge.net")
SET(PROJECT_DESCRIPTION "${PROJECT_NAME_UCASE} - Free music production software")
SET(PROJECT_COPYRIGHT   "2008-${PROJECT_YEAR} ${PROJECT_AUTHOR}")
SET(VERSION_MAJOR       "1")
SET(VERSION_MINOR       "2")
SET(VERSION_RELEASE     "0")
SET(VERSION_STAGE       "")
SET(VERSION_BUILD       "0")
SET(VERSION             "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_RELEASE}")
IF(VERSION_STAGE)
	SET(VERSION     "${VERSION}-${VERSION_STAGE}")
ENDIF()
IF(VERSION_BUILD)
	SET(VERSION     "${VERSION}.${VERSION_BUILD}")
ENDIF()

# Override version information for non-base builds
INCLUDE(VersionInfo)
INCLUDE(DetectMachine)

OPTION(WANT_ALSA	"Include ALSA (Advanced Linux Sound Architecture) support" ON)
OPTION(WANT_CALF	"Include CALF LADSPA plugins" ON)
OPTION(WANT_CAPS	"Include C* Audio Plugin Suite (LADSPA plugins)" ON)
OPTION(WANT_CARLA	"Include Carla plugin" ON)
OPTION(WANT_CMT		"Include Computer Music Toolkit LADSPA plugins" ON)
OPTION(WANT_JACK	"Include JACK (Jack Audio Connection Kit) support" ON)
OPTION(WANT_WEAKJACK	"Loosely link JACK libraries" ON)
OPTION(WANT_MP3LAME	"Include MP3/Lame support" ON)
OPTION(WANT_OGGVORBIS	"Include OGG/Vorbis support" ON)
OPTION(WANT_PULSEAUDIO	"Include PulseAudio support" ON)
OPTION(WANT_PORTAUDIO	"Include PortAudio support" ON)
OPTION(WANT_SNDIO	"Include sndio support" ON)
OPTION(WANT_SOUNDIO	"Include libsoundio support" ON)
OPTION(WANT_SDL		"Include SDL (Simple DirectMedia Layer) support" ON)
OPTION(WANT_SF2		"Include SoundFont2 player plugin" ON)
OPTION(WANT_GIG		"Include GIG player plugin" ON)
OPTION(WANT_STK		"Include Stk (Synthesis Toolkit) support" ON)
OPTION(WANT_SWH		"Include Steve Harris's LADSPA plugins" ON)
OPTION(WANT_TAP		"Include Tom's Audio Processing LADSPA plugins" ON)
OPTION(WANT_VST		"Include VST support" ON)
OPTION(WANT_VST_32	"Include 32-bit VST support" ON)
OPTION(WANT_VST_64	"Include 64-bit VST support" ON)
OPTION(WANT_WINMM	"Include WinMM MIDI support" OFF)
OPTION(WANT_DEBUG_FPE	"Debug floating point exceptions" OFF)


IF(LMMS_BUILD_APPLE)
	# Fix linking on 10.14+. See issue #4762 on github
	LINK_DIRECTORIES(/usr/local/lib)
	SET(WANT_SOUNDIO OFF)
	SET(WANT_ALSA OFF)
	SET(WANT_PULSEAUDIO OFF)
	SET(WANT_VST OFF)
	SET(STATUS_ALSA "<not supported on this platform>")
	SET(STATUS_PULSEAUDIO "<not supported on this platform>")
	SET(STATUS_APPLEMIDI "OK")
ELSE(LMMS_BUILD_APPLE)
	SET(STATUS_APPLEMIDI "<not supported on this platform>")
ENDIF(LMMS_BUILD_APPLE)


IF(LMMS_BUILD_WIN32)
	SET(WANT_ALSA OFF)
	SET(WANT_JACK OFF)
	SET(WANT_PULSEAUDIO OFF)
	SET(WANT_SNDIO OFF)
	SET(WANT_SOUNDIO OFF)
	SET(WANT_WINMM ON)
	SET(LMMS_HAVE_WINMM TRUE)
	SET(STATUS_ALSA "<not supported on this platform>")
	SET(STATUS_JACK "<not supported on this platform>")
	SET(STATUS_PULSEAUDIO "<not supported on this platform>")
	SET(STATUS_SOUNDIO "<disabled in this release>")
	SET(STATUS_WINMM "OK")
	SET(STATUS_APPLEMIDI "<not supported on this platform>")
ELSE(LMMS_BUILD_WIN32)
	SET(STATUS_WINMM "<not supported on this platform>")
ENDIF(LMMS_BUILD_WIN32)


# TODO: Fix linking issues with msys debug builds
IF(LMMS_BUILD_MSYS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
	SET(WANT_GIG OFF)
	SET(WANT_STK OFF)
	SET(WANT_SWH OFF)
	SET(STATUS_GIG "not built as requested")
	SET(STATUS_STK "not built as requested")
ENDIF()

SET(CMAKE_CXX_STANDARD_REQUIRED ON)

CHECK_INCLUDE_FILES(stdint.h LMMS_HAVE_STDINT_H)
CHECK_INCLUDE_FILES(stdlib.h LMMS_HAVE_STDLIB_H)
CHECK_INCLUDE_FILES(pthread.h LMMS_HAVE_PTHREAD_H)
CHECK_INCLUDE_FILES(semaphore.h LMMS_HAVE_SEMAPHORE_H)
CHECK_INCLUDE_FILES(unistd.h LMMS_HAVE_UNISTD_H)
CHECK_INCLUDE_FILES(sys/types.h LMMS_HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILES(sys/ipc.h LMMS_HAVE_SYS_IPC_H)
CHECK_INCLUDE_FILES(sys/shm.h LMMS_HAVE_SYS_SHM_H)
CHECK_INCLUDE_FILES(sys/time.h LMMS_HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILES(sys/times.h LMMS_HAVE_SYS_TIMES_H)
CHECK_INCLUDE_FILES(sched.h LMMS_HAVE_SCHED_H)
CHECK_INCLUDE_FILES(sys/soundcard.h LMMS_HAVE_SYS_SOUNDCARD_H)
CHECK_INCLUDE_FILES(soundcard.h LMMS_HAVE_SOUNDCARD_H)
CHECK_INCLUDE_FILES(fcntl.h LMMS_HAVE_FCNTL_H)
CHECK_INCLUDE_FILES(sys/ioctl.h LMMS_HAVE_SYS_IOCTL_H)
CHECK_INCLUDE_FILES(ctype.h LMMS_HAVE_CTYPE_H)
CHECK_INCLUDE_FILES(string.h LMMS_HAVE_STRING_H)
CHECK_INCLUDE_FILES(process.h LMMS_HAVE_PROCESS_H)
CHECK_INCLUDE_FILES(locale.h LMMS_HAVE_LOCALE_H)

LIST(APPEND CMAKE_PREFIX_PATH "${CMAKE_INSTALL_PREFIX}")

FIND_PACKAGE(Qt5 COMPONENTS Core Gui Widgets Xml REQUIRED)
FIND_PACKAGE(Qt5 COMPONENTS LinguistTools QUIET)

INCLUDE_DIRECTORIES(
	${Qt5Core_INCLUDE_DIRS}
	${Qt5Gui_INCLUDE_DIRS}
	${Qt5Widgets_INCLUDE_DIRS}
	${Qt5Xml_INCLUDE_DIRS}
)

SET(QT_LIBRARIES
	Qt5::Core
	Qt5::Gui
	Qt5::Widgets
	Qt5::Xml
)

IF(LMMS_BUILD_LINUX AND WANT_VST)
    FIND_PACKAGE(Qt5 COMPONENTS X11Extras REQUIRED)
    LIST(APPEND QT_LIBRARIES Qt5::X11Extras)
ENDIF()

# Resolve Qt5::qmake to full path for use in packaging scripts
GET_TARGET_PROPERTY(QT_QMAKE_EXECUTABLE "${Qt5Core_QMAKE_EXECUTABLE}" IMPORTED_LOCATION)

FIND_PACKAGE(Qt5Test)
SET(QT_QTTEST_LIBRARY Qt5::Test)

# check for libsndfile
FIND_PACKAGE(SndFile REQUIRED)
IF(NOT SNDFILE_FOUND)
	MESSAGE(FATAL_ERROR "LMMS requires libsndfile1 and libsndfile1-dev >= 1.0.18 - please install, remove CMakeCache.txt and try again!")
ENDIF()
# check if we can use SF_SET_COMPRESSION_LEVEL
IF(NOT SNDFILE_VERSION VERSION_LESS 1.0.26)
	SET(LMMS_HAVE_SF_COMPLEVEL TRUE)
ENDIF()

IF(WANT_CALF)
	SET(LMMS_HAVE_CALF TRUE)
	SET(STATUS_CALF "OK")
ELSE(WANT_CALF)
	SET(STATUS_CALF "not built as requested")
ENDIF(WANT_CALF)

IF(WANT_CAPS)
	SET(LMMS_HAVE_CAPS TRUE)
	SET(STATUS_CAPS "OK")
ELSE(WANT_CAPS)
	SET(STATUS_CAPS "not built as requested")
ENDIF(WANT_CAPS)

IF(WANT_CMT)
	SET(LMMS_HAVE_CMT TRUE)
	SET(STATUS_CMT "OK")
ELSE(WANT_CMT)
	SET(STATUS_CMT "not built as requested")
ENDIF(WANT_CMT)

IF(WANT_SWH)
	SET(LMMS_HAVE_SWH TRUE)
	SET(STATUS_SWH "OK")
ELSE(WANT_SWH)
	SET(STATUS_SWH "not built as requested")
ENDIF(WANT_SWH)

IF(WANT_TAP)
	SET(LMMS_HAVE_TAP TRUE)
	SET(STATUS_TAP "OK")
ELSE(WANT_TAP)
	SET(STATUS_TAP "not built as requested")
ENDIF(WANT_TAP)


# check for CARLA
IF(WANT_CARLA)
	PKG_CHECK_MODULES(CARLA carla-native-plugin)
	# look for carla under old name
	IF(NOT CARLA_FOUND)
		PKG_CHECK_MODULES(CARLA carla-standalone>=1.9.5)
	ENDIF()
	IF(CARLA_FOUND)
		SET(LMMS_HAVE_CARLA TRUE)
		SET(STATUS_CARLA "OK")
	ELSE(CARLA_FOUND)
		SET(STATUS_CARLA "not found, please install the latest carla")
	ENDIF(CARLA_FOUND)
ENDIF(WANT_CARLA)


# check for SDL2
IF(WANT_SDL)
	SET(SDL2_BUILDING_LIBRARY TRUE)
	FIND_PACKAGE(SDL2)
	IF(SDL2_FOUND)
		SET(LMMS_HAVE_SDL TRUE)
		SET(LMMS_HAVE_SDL2 TRUE)
		SET(STATUS_SDL "OK, using SDL2")
		SET(SDL_INCLUDE_DIR "")
		SET(SDL_LIBRARY "")
	ELSE()
		SET(SDL2_INCLUDE_DIR "")
		SET(SDL2_LIBRARY "")
	ENDIF()
ENDIF()

# fallback to SDL1
IF(WANT_SDL AND NOT LMMS_HAVE_SDL2)
	# Fallback to SDL1.2
	SET(SDL_BUILDING_LIBRARY TRUE)
	FIND_PACKAGE(SDL)
	IF(SDL_FOUND)
		SET(LMMS_HAVE_SDL TRUE)
		SET(STATUS_SDL "OK, using SDL1.2")
		# fix mingw since 53abd65
		IF(NOT SDL_INCLUDE_DIR)
			SET(SDL_INCLUDE_DIR "${CMAKE_FIND_ROOT_PATH}/include")
		ENDIF()

	ELSE()
		SET(STATUS_SDL "not found, please install libsdl2-dev (or similar) "
			"if you require SDL support")
		SET(SDL_INCLUDE_DIR "")
		SET(SDL_LIBRARY "")
	ENDIF()
ENDIF()

# check for Stk
IF(WANT_STK)
	FIND_PACKAGE(STK)
	IF(STK_FOUND)
		SET(LMMS_HAVE_STK TRUE)
		SET(STATUS_STK "OK")
	ELSE(STK_FOUND)
		SET(STK_INCLUDE_DIR "")
		SET(STATUS_STK "not found, please install libstk0-dev (or similar) "
			"if you require the Mallets instrument")
	ENDIF(STK_FOUND)
ENDIF(WANT_STK)


# check for PortAudio
IF(WANT_PORTAUDIO)
	FIND_PACKAGE(Portaudio)
	IF(PORTAUDIO_FOUND)
		SET(LMMS_HAVE_PORTAUDIO TRUE)
		SET(STATUS_PORTAUDIO "OK")
	ELSE(PORTAUDIO_FOUND)
		SET(STATUS_PORTAUDIO "not found, please install portaudio19-dev (or similar, version >= 1.9) "
			"if you require PortAudio support")
	ENDIF(PORTAUDIO_FOUND)
ENDIF(WANT_PORTAUDIO)

# check for libsoundio
IF(WANT_SOUNDIO)
  FIND_PACKAGE(SoundIo)
  IF(SOUNDIO_FOUND)
	SET(LMMS_HAVE_SOUNDIO TRUE)
	SET(STATUS_SOUNDIO "OK")
	INCLUDE_DIRECTORIES("${SOUNDIO_INCLUDE_DIR}")
  ELSE(SOUNDIO_FOUND)
	SET(SOUNDIO_INCLUDE_DIR "")
	SET(STATUS_SOUNDIO "not found, please install libsoundio if you require libsoundio support")
	SET(SOUNDIO_LIBRARY "")
  ENDIF(SOUNDIO_FOUND)
ENDIF(WANT_SOUNDIO)


# check for PulseAudio
IF(WANT_PULSEAUDIO)
	FIND_PACKAGE(PulseAudio)
	IF(PULSEAUDIO_FOUND)
		SET(LMMS_HAVE_PULSEAUDIO TRUE)
		SET(STATUS_PULSEAUDIO "OK")
	ELSE(PULSEAUDIO_FOUND)
		SET(STATUS_PULSEAUDIO "not found, please install libpulse-dev (or similar) "
			"if you require PulseAudio support")
	ENDIF(PULSEAUDIO_FOUND)
ENDIF(WANT_PULSEAUDIO)
IF(NOT LMMS_HAVE_PULSEAUDIO)
	SET(PULSEAUDIO_INCLUDE_DIR "")
	SET(PULSEAUDIO_LIBRARIES "")
ENDIF(NOT LMMS_HAVE_PULSEAUDIO)


# check for MP3/Lame-libraries
IF(WANT_MP3LAME)
	FIND_PACKAGE(Lame)
	IF(LAME_FOUND)
		SET(LMMS_HAVE_MP3LAME TRUE)
		SET(STATUS_MP3LAME "OK")
	ELSE(LAME_FOUND)
		SET(STATUS_MP3LAME "not found, please install libmp3lame-dev (or similar)")
		SET(LAME_LIBRARIES "")
		SET(LAME_INCLUDE_DIRS "")
	ENDIF(LAME_FOUND)
ELSE(WANT_MP3LAME)
	SET(STATUS_MP3LAME "Disabled for build")
ENDIF(WANT_MP3LAME)

# check for OGG/Vorbis-libraries
IF(WANT_OGGVORBIS)
	FIND_PACKAGE(OggVorbis)
	IF(OGGVORBIS_FOUND)
		SET(LMMS_HAVE_OGGVORBIS TRUE)
		SET(STATUS_OGGVORBIS "OK")
	ELSE(OGGVORBIS_FOUND)
		SET(STATUS_OGGVORBIS "not found, libogg-dev and libvorbis-dev (or similar) "
			"is highly recommended")
	ENDIF(OGGVORBIS_FOUND)
ENDIF(WANT_OGGVORBIS)


# check whether to enable OSS-support
IF(LMMS_HAVE_SOUNDCARD_H OR LMMS_HAVE_SYS_SOUNDCARD_H)
	SET(LMMS_HAVE_OSS TRUE)
	SET(STATUS_OSS "OK")
ELSE(LMMS_HAVE_SOUNDCARD_H OR LMMS_HAVE_SYS_SOUNDCARD_H)
	SET(STATUS_OSS "<not found or not supported on this platform>")
ENDIF(LMMS_HAVE_SOUNDCARD_H OR LMMS_HAVE_SYS_SOUNDCARD_H)


# check for ALSA
IF(WANT_ALSA)
	FIND_PACKAGE(Alsa)
	IF(ALSA_FOUND)
		SET(LMMS_HAVE_ALSA TRUE)
		SET(STATUS_ALSA "OK")
	ELSE(ALSA_FOUND)
		SET(STATUS_ALSA "not found, please install libasound2-dev (or similar) "
			"if you require ALSA support")
	ENDIF(ALSA_FOUND)
ENDIF(WANT_ALSA)
IF(NOT LMMS_HAVE_ALSA)
	SET(ASOUND_LIBRARY "")
ENDIF(NOT LMMS_HAVE_ALSA)


# check for JACK
IF(WANT_JACK)
	PKG_CHECK_MODULES(JACK jack>=0.77)
	IF(JACK_FOUND)
		IF(WANT_WEAKJACK)
			SET(LMMS_HAVE_WEAKJACK TRUE)
			SET(WEAKJACK_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/src/3rdparty/weakjack/weakjack)
			SET(STATUS_JACK "OK (weak linking enabled)")
			# use dlsym instead
			SET(JACK_LIBRARIES ${CMAKE_DL_LIBS})
		ELSE()
			SET(STATUS_JACK "OK")
		ENDIF()
		SET(LMMS_HAVE_JACK TRUE)
	ELSE(JACK_FOUND)
		SET(JACK_INCLUDE_DIRS "")
		SET(STATUS_JACK "not found, please install libjack0.100.0-dev (or similar) "
			"if you require JACK support")
	ENDIF(JACK_FOUND)
ENDIF(WANT_JACK)

# check for FFTW3F-library
FIND_PACKAGE(FFTW COMPONENTS fftw3f REQUIRED)

# check for FLTK
FIND_PACKAGE(FLTK)
IF(FLTK_FOUND)
	SET(STATUS_ZYN "OK")
ELSE()
	SET(STATUS_ZYN "not found, please install fltk")
ENDIF()

# check for Fluidsynth
IF(WANT_SF2)
	PKG_CHECK_MODULES(FLUIDSYNTH fluidsynth>=1.0.7)
	IF(FLUIDSYNTH_FOUND)
		SET(LMMS_HAVE_FLUIDSYNTH TRUE)
		SET(STATUS_FLUIDSYNTH "OK")
	ELSE(FLUIDSYNTH_FOUND)
		SET(STATUS_FLUIDSYNTH "not found, libfluidsynth-dev (or similar)"
			"is highly recommended")
	ENDIF(FLUIDSYNTH_FOUND)
ENDIF(WANT_SF2)

# check for libgig
If(WANT_GIG)
	PKG_CHECK_MODULES(GIG gig)
	IF(GIG_FOUND)
		SET(LMMS_HAVE_GIG TRUE)
		SET(STATUS_GIG "OK")
	ELSE(GIG_FOUND)
		SET(STATUS_GIG "not found, libgig needed for decoding .gig files")
	ENDIF(GIG_FOUND)
ENDIF(WANT_GIG)

# check for pthreads
IF(LMMS_BUILD_LINUX OR LMMS_BUILD_APPLE OR LMMS_BUILD_OPENBSD)
	FIND_PACKAGE(Threads)
ENDIF(LMMS_BUILD_LINUX OR LMMS_BUILD_APPLE OR LMMS_BUILD_OPENBSD)

# check for sndio (roaraudio won't work yet)
IF(WANT_SNDIO)
	FIND_PACKAGE(Sndio)
	IF(SNDIO_FOUND)
		SET(LMMS_HAVE_SNDIO TRUE)
        	SET(STATUS_SNDIO "OK")
    	ELSE()
        	SET(STATUS_SNDIO "<not found or not supported on this platform>")
	ENDIF(SNDIO_FOUND)
ENDIF(WANT_SNDIO)

# check for WINE
IF(WANT_VST)
	FIND_PACKAGE(Wine)
	IF(WINE_FOUND)
		SET(LMMS_SUPPORT_VST TRUE)
		IF(WINE_LIBRARY_FIX)
			SET(STATUS_VST "OK, with workaround linking ${WINE_LIBRARY_FIX}")
		ELSE()
			SET(STATUS_VST "OK")
		ENDIF()
	ELSEIF(WANT_VST_NOWINE)
		SET(LMMS_SUPPORT_VST TRUE)
		SET(STATUS_VST "OK")
	ELSE(WINE_FOUND)
		SET(STATUS_VST "not found, please install (lib)wine-dev (or similar) - 64 bit systems additionally need gcc-multilib and g++-multilib")
	ENDIF(WINE_FOUND)
ENDIF(WANT_VST)
IF(LMMS_BUILD_WIN32)
	SET(LMMS_SUPPORT_VST TRUE)
	SET(STATUS_VST "OK")
ENDIF(LMMS_BUILD_WIN32)

IF(WANT_DEBUG_FPE)
	IF(LMMS_BUILD_LINUX OR LMMS_BUILD_APPLE)
		SET(LMMS_DEBUG_FPE TRUE)
		SET (STATUS_DEBUG_FPE "Enabled")
	ELSE()
		SET (STATUS_DEBUG_FPE "Wanted but disabled due to unsupported platform")
	ENDIF()
ELSE()
	SET (STATUS_DEBUG_FPE "Disabled")
ENDIF(WANT_DEBUG_FPE)

# check for libsamplerate
FIND_PACKAGE(Samplerate 0.1.8 MODULE REQUIRED)

# set compiler flags
IF(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	SET(WERROR_FLAGS "-Wall -Werror=unused-function -Wno-sign-compare -Wno-strict-overflow")
	OPTION(USE_WERROR "Add -werror to the build flags. Stops the build on warnings" OFF)
	IF(${USE_WERROR})
		SET(WERROR_FLAGS "${WERROR_FLAGS} -Werror")
	ENDIF()

	# Due to a regression in gcc-4.8.X, we need to disable array-bounds check
	IF (CMAKE_COMPILER_IS_GNUCXX AND ((CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL "4.8.0") OR (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "4.8.0") OR LMMS_BUILD_WIN32))
		SET(WERROR_FLAGS "${WERROR_FLAGS} -Wno-array-bounds")
	ENDIF()
ELSEIF(MSVC)
	# Remove any existing /W flags
	STRING(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
	SET(WERROR_FLAGS "/W2")
	IF(${USE_WERROR})
		SET(WERROR_FLAGS "${WERROR_FLAGS} /WX")
	ENDIF()
ENDIF()


IF(NOT CMAKE_BUILD_TYPE)
	message(STATUS "Setting build type to 'Release' as none was specified.")
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
	# Set the possible values of build type for cmake-gui
	SET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
		"MinSizeRel" "RelWithDebInfo")
ENDIF()

SET(CMAKE_C_FLAGS "${WERROR_FLAGS} ${CMAKE_C_FLAGS}")
SET(CMAKE_CXX_FLAGS "${WERROR_FLAGS} ${CMAKE_CXX_FLAGS}")
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DLMMS_DEBUG")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DLMMS_DEBUG")


# people simply updating git will still have this and mess up build with it
FILE(REMOVE include/lmmsconfig.h)

FILE(GLOB LMMS_INCLUDES "${CMAKE_SOURCE_DIR}/include/*.h")
LIST(SORT LMMS_INCLUDES)

# Get list of all committers from git history, ordered by number of commits.
# The CONTRIBUTORS file is used by AboutDialog. This information can be provided
# with -DCONTRIBUTORS=/path/to/CONTRIBUTORS instead. For instance, to generate
# this file for version 1.1.3, the command is:
# 	git shortlog -sne v1.1.3 | cut -c8-
FIND_PACKAGE(Git)
IF(GIT_FOUND AND NOT CONTRIBUTORS)
	SET(CONTRIBUTORS "${CMAKE_BINARY_DIR}/CONTRIBUTORS")
	EXECUTE_PROCESS(
		COMMAND "${GIT_EXECUTABLE}" shortlog -sne
		COMMAND cut -c8-
		OUTPUT_FILE "${CONTRIBUTORS}"
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		TIMEOUT 1)
ENDIF()

# we somehow have to make LMMS-binary depend on MOC-files
ADD_FILE_DEPENDENCIES("${CMAKE_BINARY_DIR}/lmmsconfig.h")

IF(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	IF(WIN32)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes")
	ELSE(WIN32)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -DPIC")
	ENDIF(WIN32)
ENDIF()

# use ccache
include(CompileCache)

# make sub-directories
ADD_SUBDIRECTORY(cmake)
ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(plugins)
ADD_SUBDIRECTORY(tests)
ADD_SUBDIRECTORY(data)
ADD_SUBDIRECTORY(doc)

# install tasks
ADD_SUBDIRECTORY(cmake/install)

ADD_CUSTOM_COMMAND(OUTPUT "${CMAKE_BINARY_DIR}/lmms.1.gz"
				COMMAND gzip -c ${CMAKE_SOURCE_DIR}/doc/lmms.1 > ${CMAKE_BINARY_DIR}/lmms.1.gz
				DEPENDS "${CMAKE_SOURCE_DIR}/doc/lmms.1"
				COMMENT "Generating lmms.1.gz"
				VERBATIM)


ADD_CUSTOM_TARGET(manpage ALL
				DEPENDS "${CMAKE_BINARY_DIR}/lmms.1.gz")

# install headers

IF(LMMS_BUILD_LINUX)
	INSTALL(FILES ${LMMS_INCLUDES}
		"${CMAKE_BINARY_DIR}/lmmsconfig.h"
		"${CMAKE_BINARY_DIR}/lmmsversion.h"
		"${CMAKE_SOURCE_DIR}/src/gui/embed.cpp"
		DESTINATION "${CMAKE_INSTALL_PREFIX}/include/lmms/")
ENDIF(LMMS_BUILD_LINUX)

# package ZynAddSubFX into win32 build
IF(LMMS_BUILD_WIN32)
	IF(EXISTS "${CMAKE_SOURCE_DIR}/extras")
		ADD_SUBDIRECTORY("${CMAKE_SOURCE_DIR}/extras/data/presets")
		FILE(GLOB ZASF_BINARIES
			"${CMAKE_SOURCE_DIR}/extras/plugins/zynaddsubfx/zynaddsubfx.dll"
			"${CMAKE_SOURCE_DIR}/extras/plugins/zynaddsubfx/remote_zynaddsubfx.exe")
		LIST(SORT ZASF_BINARIES)
		INSTALL(FILES "${ZASF_BINARIES}" DESTINATION "${PLUGIN_DIR}")
	ENDIF(EXISTS "${CMAKE_SOURCE_DIR}/extras")
ENDIF(LMMS_BUILD_WIN32)

#
# add distclean-target
#
ADD_CUSTOM_TARGET(distclean
			COMMAND make clean
			COMMAND rm -rf `find -name cmake_install.cmake` `find -name Makefile` `find -type d -name CMakeFiles` CMakeCache.txt lmmsconfig.h lmms.1.gz)

#
# add tarball-target
#
SET(TMP "lmms-${VERSION}")
ADD_CUSTOM_TARGET(dist
			COMMAND make clean
			COMMAND rm -rf "${TMP}"
			COMMAND mkdir -p "${TMP}"
			COMMAND cp CMakeLists.txt LICENSE.txt INSTALL.txt README.md "${TMP}"
			COMMAND cp -r buildtools cmake data doc include plugins src "${TMP}"
			COMMAND rm -rf `find "${TMP}" -name cmake_install.cmake` `find "${TMP}" -name Makefile` `find "${TMP}" -type d -name CMakeFiles` "${TMP}/CMakeCache.txt"
			COMMAND tar cjf lmms-${VERSION}-src.tar.bz2 "${TMP}"
			COMMAND rm -rf "${TMP}")


#
# add uninstall-target
#
ADD_CUSTOM_TARGET(uninstall
	COMMAND ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake"
)


#
# display configuration information
#

MESSAGE("\n"
"Installation Summary\n"
"--------------------\n"
"* Install Directory           : ${CMAKE_INSTALL_PREFIX}\n"
)

MESSAGE(
"Supported audio interfaces\n"
"--------------------------\n"
"* ALSA                        : ${STATUS_ALSA}\n"
"* JACK                        : ${STATUS_JACK}\n"
"* OSS                         : ${STATUS_OSS}\n"
"* Sndio                       : ${STATUS_SNDIO}\n"
"* PortAudio                   : ${STATUS_PORTAUDIO}\n"
"* libsoundio                  : ${STATUS_SOUNDIO}\n"
"* PulseAudio                  : ${STATUS_PULSEAUDIO}\n"
"* SDL                         : ${STATUS_SDL}\n"
)

MESSAGE(
"Supported MIDI interfaces\n"
"-------------------------\n"
"* ALSA                        : ${STATUS_ALSA}\n"
"* OSS                         : ${STATUS_OSS}\n"
"* Sndio                       : ${STATUS_SNDIO}\n"
"* JACK                        : ${STATUS_JACK}\n"
"* WinMM                       : ${STATUS_WINMM}\n"
"* AppleMidi                   : ${STATUS_APPLEMIDI}\n"
)

MESSAGE(
"Supported file formats for project export\n"
"-----------------------------------------\n"
"* WAVE                        : OK\n"
"* FLAC                        : OK\n"
"* OGG/VORBIS                  : ${STATUS_OGGVORBIS}\n"
"* MP3/Lame                    : ${STATUS_MP3LAME}\n"
)

MESSAGE(
"Optional plugins\n"
"----------------\n"
"* ZynAddSubFX instrument      : ${STATUS_ZYN}\n"
"* Carla Patchbay & Rack       : ${STATUS_CARLA}\n"
"* SoundFont2 player           : ${STATUS_FLUIDSYNTH}\n"
"* Stk Mallets                 : ${STATUS_STK}\n"
"* VST-instrument hoster       : ${STATUS_VST}\n"
"* VST-effect hoster           : ${STATUS_VST}\n"
"* CALF LADSPA plugins         : ${STATUS_CALF}\n"
"* CAPS LADSPA plugins         : ${STATUS_CAPS}\n"
"* CMT LADSPA plugins          : ${STATUS_CMT}\n"
"* TAP LADSPA plugins          : ${STATUS_TAP}\n"
"* SWH LADSPA plugins          : ${STATUS_SWH}\n"
"* GIG player                  : ${STATUS_GIG}\n"
)

MESSAGE(
"Developer options\n"
"-----------------------------------------\n"
"* Debug FP exceptions         : ${STATUS_DEBUG_FPE}\n"
)

MESSAGE(
"\n"
"-----------------------------------------------------------------\n"
"IMPORTANT:\n"
"after installing missing packages, remove CMakeCache.txt before\n"
"running cmake again!\n"
"-----------------------------------------------------------------\n"
"\n\n")

SET(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "${BIN_DIR}")
if(MSVC)
	# We can't set this on the install time according to the configuration
	SET(CMAKE_INSTALL_DEBUG_LIBRARIES TRUE)
	SET(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
endif()
INCLUDE(InstallRequiredSystemLibraries)
