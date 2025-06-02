CMAKE_MINIMUM_REQUIRED(VERSION 3.13)
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set the given policy to NEW. If it does not exist, it will not be set. If it
# is already set to NEW (most likely due to predating the minimum required CMake
# version), a developer warning is emitted indicating that the policy need no
# longer be explicitly set.
function(enable_policy_if_exists id)
	if(POLICY "${id}")
		cmake_policy(GET "${id}" current_value)
		if(current_value STREQUAL "NEW")
			message(AUTHOR_WARNING "${id} is now set to NEW by default, and no longer needs to be explicitly set.")
		else()
			cmake_policy(SET "${id}" NEW)
		endif()
	endif()
endfunction()

enable_policy_if_exists(CMP0092) # MSVC warning flags are not in CMAKE_<LANG>_FLAGS by default.
# Needed for ccache support with MSVC
enable_policy_if_exists(CMP0141) # MSVC debug information format flags are selected by an abstraction.

PROJECT(lmms)

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})
SET(LMMS_BINARY_DIR ${CMAKE_BINARY_DIR})
SET(LMMS_SOURCE_DIR ${CMAKE_SOURCE_DIR})

# Import of windows.h breaks min()/max()
ADD_DEFINITIONS(-DNOMINMAX)

# CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES is not set correctly for MinGW until
# CMake 3.14.1, so avoid specifying system include directories on affected
# versions. Normal include directories are safe, since GCC ignores them if they
# are already in the built-in search path.
if(MINGW AND CMAKE_VERSION VERSION_LESS "3.14.1")
	set(CMAKE_NO_SYSTEM_FROM_IMPORTED TRUE)
endif()

INCLUDE(PluginList)
INCLUDE(CheckSubmodules)
INCLUDE(AddFileDependencies)
INCLUDE(CheckIncludeFiles)
INCLUDE(FindPkgConfig)
INCLUDE(GenerateExportHeader)
include(StaticDependencies)

STRING(TOUPPER          "${CMAKE_PROJECT_NAME}" PROJECT_NAME_UCASE)

SET(PROJECT_YEAR 2025)

SET(PROJECT_AUTHOR      "LMMS Developers")
SET(PROJECT_URL         "https://lmms.io")
SET(PROJECT_EMAIL       "lmms-devel@lists.sourceforge.net")
SET(PROJECT_DESCRIPTION "${PROJECT_NAME_UCASE} - Free music production software")
SET(PROJECT_COPYRIGHT   "2008-${PROJECT_YEAR} ${PROJECT_AUTHOR}")
SET(VERSION_MAJOR       "1")
SET(VERSION_MINOR       "3")
SET(VERSION_RELEASE     "0")
SET(VERSION_STAGE       "alpha")
SET(VERSION_BUILD       "")
SET(VERSION             "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_RELEASE}")
IF(VERSION_STAGE)
	SET(VERSION     "${VERSION}-${VERSION_STAGE}")
ENDIF()
IF(VERSION_BUILD)
	SET(VERSION     "${VERSION}-${VERSION_BUILD}")
ENDIF()

# Override version information for non-base builds
INCLUDE(VersionInfo)
INCLUDE(DetectMachine)

OPTION(WANT_ALSA	"Include ALSA (Advanced Linux Sound Architecture) support" ON)
OPTION(WANT_OSS 	"Include Open Sound System support" ON)
OPTION(WANT_CALF	"Include CALF LADSPA plugins" ON)
OPTION(WANT_CAPS	"Include C* Audio Plugin Suite (LADSPA plugins)" ON)
OPTION(WANT_CARLA	"Include Carla plugin" ON)
OPTION(WANT_CMT		"Include Computer Music Toolkit LADSPA plugins" ON)
OPTION(WANT_JACK	"Include JACK (Jack Audio Connection Kit) support" ON)
OPTION(WANT_WEAKJACK	"Loosely link JACK libraries" ON)
OPTION(WANT_LV2		"Include Lv2 plugins" ON)
OPTION(WANT_SUIL	"Include SUIL for LV2 plugin UIs" ON)
OPTION(WANT_MP3LAME	"Include MP3/Lame support" ON)
OPTION(WANT_OGGVORBIS	"Include OGG/Vorbis support" ON)
OPTION(WANT_PULSEAUDIO	"Include PulseAudio support" ON)
OPTION(WANT_PORTAUDIO	"Include PortAudio support" ON)
OPTION(WANT_SNDIO	"Include sndio support" ON)
OPTION(WANT_SOUNDIO	"Include libsoundio support" ON)
OPTION(WANT_SDL		"Include SDL (Simple DirectMedia Layer) support" ON)
OPTION(WANT_SF2		"Include SoundFont2 player plugin" ON)
OPTION(WANT_GIG		"Include GIG player plugin" ON)
option(WANT_SID		"Include Sid instrument" ON)
OPTION(WANT_STK		"Include Stk (Synthesis Toolkit) support" ON)
OPTION(WANT_SWH		"Include Steve Harris's LADSPA plugins" ON)
OPTION(WANT_TAP		"Include Tom's Audio Processing LADSPA plugins" ON)
OPTION(WANT_VST		"Include VST support" ON)
OPTION(WANT_VST_32	"Include 32-bit Windows VST support" ON)
OPTION(WANT_VST_64	"Include 64-bit Windows VST support" ON)
OPTION(WANT_WINMM	"Include WinMM MIDI support" OFF)
OPTION(WANT_DEBUG_FPE	"Debug floating point exceptions" OFF)
option(WANT_DEBUG_ASAN	"Enable AddressSanitizer" OFF)
option(WANT_DEBUG_TSAN	"Enable ThreadSanitizer" OFF)
option(WANT_DEBUG_MSAN	"Enable MemorySanitizer" OFF)
option(WANT_DEBUG_UBSAN	"Enable UndefinedBehaviorSanitizer" OFF)
option(WANT_DEBUG_GPROF	"Enable gprof profiler" OFF)
OPTION(BUNDLE_QT_TRANSLATIONS	"Install Qt translation files for LMMS" OFF)
option(WANT_DEBUG_CPACK "Show detailed logs for packaging commands" OFF)
option(WANT_CPACK_TARBALL "Request CPack to create a tarball instead of an installer" OFF)


IF(LMMS_BUILD_APPLE)
	# Fix linking on 10.14+. See issue #4762 on github
	LINK_DIRECTORIES("${APPLE_PREFIX}/lib")
	SET(WANT_SOUNDIO OFF)
	SET(WANT_ALSA OFF)
	SET(WANT_OSS OFF)
	SET(WANT_PULSEAUDIO OFF)
	SET(WANT_VST OFF)
	SET(STATUS_ALSA "<not supported on this platform>")
	SET(STATUS_OSS "<not supported on this platform>")
	SET(STATUS_PULSEAUDIO "<not supported on this platform>")
	SET(STATUS_APPLEMIDI "OK")
ELSE(LMMS_BUILD_APPLE)
	SET(STATUS_APPLEMIDI "<not supported on this platform>")
ENDIF(LMMS_BUILD_APPLE)


IF(LMMS_BUILD_WIN32)
	SET(WANT_ALSA OFF)
	SET(WANT_OSS OFF)
	SET(WANT_PULSEAUDIO OFF)
	SET(WANT_SNDIO OFF)
	SET(WANT_SOUNDIO OFF)
	SET(WANT_WINMM ON)
	SET(BUNDLE_QT_TRANSLATIONS ON)
	SET(LMMS_HAVE_WINMM TRUE)
	if(NOT LMMS_BUILD_WIN64)
		set(WANT_VST_64 OFF)
	endif()
	SET(STATUS_ALSA "<not supported on this platform>")
	SET(STATUS_OSS "<not supported on this platform>")
	SET(STATUS_PULSEAUDIO "<not supported on this platform>")
	SET(STATUS_SOUNDIO "<disabled in this release>")
	SET(STATUS_SNDIO "<not supported on this platform>")
	SET(STATUS_WINMM "OK")
	SET(STATUS_APPLEMIDI "<not supported on this platform>")
ELSE(LMMS_BUILD_WIN32)
	SET(STATUS_WINMM "<not supported on this platform>")
ENDIF(LMMS_BUILD_WIN32)

SET(CMAKE_CXX_STANDARD_REQUIRED ON)

CHECK_INCLUDE_FILES(pthread.h LMMS_HAVE_PTHREAD_H)
CHECK_INCLUDE_FILES(semaphore.h LMMS_HAVE_SEMAPHORE_H)
CHECK_INCLUDE_FILES(unistd.h LMMS_HAVE_UNISTD_H)
CHECK_INCLUDE_FILES(sys/types.h LMMS_HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILES(sys/ipc.h LMMS_HAVE_SYS_IPC_H)
CHECK_INCLUDE_FILES(sys/time.h LMMS_HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILES(sys/times.h LMMS_HAVE_SYS_TIMES_H)
CHECK_INCLUDE_FILES(sys/prctl.h LMMS_HAVE_SYS_PRCTL_H)
CHECK_INCLUDE_FILES(sched.h LMMS_HAVE_SCHED_H)
CHECK_INCLUDE_FILES(sys/soundcard.h LMMS_HAVE_SYS_SOUNDCARD_H)
CHECK_INCLUDE_FILES(soundcard.h LMMS_HAVE_SOUNDCARD_H)
CHECK_INCLUDE_FILES(fcntl.h LMMS_HAVE_FCNTL_H)
CHECK_INCLUDE_FILES(sys/ioctl.h LMMS_HAVE_SYS_IOCTL_H)
CHECK_INCLUDE_FILES(ctype.h LMMS_HAVE_CTYPE_H)
CHECK_INCLUDE_FILES(string.h LMMS_HAVE_STRING_H)
CHECK_INCLUDE_FILES(process.h LMMS_HAVE_PROCESS_H)
CHECK_INCLUDE_FILES(locale.h LMMS_HAVE_LOCALE_H)

include(CheckLibraryExists)
check_library_exists(rt shm_open "" LMMS_HAVE_LIBRT)

LIST(APPEND CMAKE_PREFIX_PATH "${CMAKE_INSTALL_PREFIX}")

FIND_PACKAGE(Qt5 5.9.0 COMPONENTS Core Gui Widgets Xml Svg REQUIRED)
FIND_PACKAGE(Qt5 COMPONENTS LinguistTools QUIET)

include_directories(SYSTEM
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
	Qt5::Svg
)

IF(LMMS_BUILD_LINUX AND WANT_VST)
    FIND_PACKAGE(Qt5 COMPONENTS X11Extras REQUIRED)
    LIST(APPEND QT_LIBRARIES Qt5::X11Extras)
ENDIF()

# Resolve Qt5::qmake to full path for use in packaging scripts
GET_TARGET_PROPERTY(QT_QMAKE_EXECUTABLE "${Qt5Core_QMAKE_EXECUTABLE}" IMPORTED_LOCATION)

# Find the location of Qt translation files
execute_process(COMMAND ${QT_QMAKE_EXECUTABLE} -query QT_INSTALL_TRANSLATIONS
	OUTPUT_VARIABLE QT_TRANSLATIONS_DIR
	OUTPUT_STRIP_TRAILING_WHITESPACE
	ERROR_QUIET
)
IF(EXISTS "${QT_TRANSLATIONS_DIR}")
	MESSAGE("-- Found Qt translations in ${QT_TRANSLATIONS_DIR}")
	ADD_DEFINITIONS("-DQT_TRANSLATIONS_DIR=\"${QT_TRANSLATIONS_DIR}\"")
ENDIF()

FIND_PACKAGE(Qt5Test)
SET(QT_QTTEST_LIBRARY Qt5::Test)

# check for libsndfile
FIND_PACKAGE(SndFile REQUIRED)
IF(SNDFILE_FOUND)
	IF(SndFile_VERSION VERSION_GREATER_EQUAL "1.1.0")
		SET(LMMS_HAVE_SNDFILE_MP3 TRUE)
	ELSE()
		MESSAGE("libsndfile version is < 1.1.0; MP3 import disabled")
		SET(LMMS_HAVE_SNDFILE_MP3 FALSE)
	ENDIF()
ELSE()
	MESSAGE(FATAL_ERROR "LMMS requires libsndfile1 and libsndfile1-dev >= 1.0.18 - please install, remove CMakeCache.txt and try again!")
ENDIF()
# check if we can use SFC_SET_COMPRESSION_LEVEL
INCLUDE(CheckCXXSourceCompiles)
CHECK_CXX_SOURCE_COMPILES(
	"#include <sndfile.h>
		int main() {SFC_SET_COMPRESSION_LEVEL;}"
	LMMS_HAVE_SF_COMPLEVEL
)

# check for perl
if(LMMS_BUILD_APPLE)
	# Prefer system perl over Homebrew, MacPorts, etc
	set(Perl_ROOT "/usr/bin")
endif()
find_package(Perl)

if(WANT_LV2)
	if(PKG_CONFIG_FOUND)
		pkg_check_modules(LV2 lv2)
	endif()

	find_package(Lilv)
	if(Lilv_FOUND)
		set(LILV_LIBRARIES Lilv::lilv)
	endif()

	# Ensure both dependencies are found
	if(NOT LV2_FOUND)
		set(STATUS_LV2 "not found, install lv2 or set PKG_CONFIG_PATH appropriately")
	elseif(NOT Lilv_FOUND)
		set(STATUS_LV2 "not found, install lilv or set PKG_CONFIG_PATH appropriately")
	else()
		set(LMMS_HAVE_LV2 TRUE)
		set(STATUS_LV2 "OK")
	endif()
else()
	set(STATUS_LV2 "not built as requested")
endif()

IF(WANT_SUIL)
	IF(PKG_CONFIG_FOUND)
		PKG_CHECK_MODULES(SUIL suil-0)
		IF(SUIL_FOUND)
			SET(LMMS_HAVE_SUIL TRUE)
			SET(STATUS_SUIL "OK")
			find_package(SuilModules)
		ELSE()
			SET(STATUS_SUIL "not found, install it or set PKG_CONFIG_PATH appropriately")
		ENDIF()
	ELSE()
		SET(STATUS_SUIL "not found, requires pkg-config")
	ENDIF()
ELSE(WANT_SUIL)
	SET(STATUS_SUIL "not built as requested")
ENDIF(WANT_SUIL)

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
	IF(PERL_FOUND)
		SET(LMMS_HAVE_SWH TRUE)
		SET(STATUS_SWH "OK")
	ELSE()
		SET(STATUS_SWH "Skipping, perl is missing")
	ENDIF()
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
		SET(LMMS_HAVE_WEAKCARLA TRUE)
		SET(STATUS_CARLA "OK (weak linking enabled)")
	ENDIF(CARLA_FOUND)
ENDIF(WANT_CARLA)


# check for SDL2
IF(WANT_SDL)
	FIND_PACKAGE(SDL2)
	IF(SDL2_FOUND)
		SET(LMMS_HAVE_SDL TRUE)
		SET(STATUS_SDL "OK")
		SET(SDL2_LIBRARY "SDL2::SDL2")
	ELSE()
		SET(STATUS_SDL "not found, please install libsdl2-dev (or similar) if you require SDL support")
		SET(SDL2_LIBRARY "")
	ENDIF()
ENDIF()

# check for Sid
if(WANT_SID)
	if(PERL_FOUND)
		set(LMMS_HAVE_SID TRUE)
		set(STATUS_SID "OK")
	else()
		set(STATUS_SID "not found, please install perl if you require the Sid instrument")
	endif()
endif()

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
	IF(Portaudio_FOUND)
		SET(LMMS_HAVE_PORTAUDIO TRUE)
		SET(STATUS_PORTAUDIO "OK")
	ELSE()
		SET(STATUS_PORTAUDIO "not found, please install portaudio19-dev (or similar, version >= 1.9) "
			"if you require PortAudio support")
	ENDIF()
ENDIF(WANT_PORTAUDIO)

# check for libsoundio
IF(WANT_SOUNDIO)
  FIND_PACKAGE(SoundIo)
  IF(SOUNDIO_FOUND)
	SET(LMMS_HAVE_SOUNDIO TRUE)
	SET(STATUS_SOUNDIO "OK")
	include_directories(SYSTEM "${SOUNDIO_INCLUDE_DIR}")
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


# check for OSS
IF(WANT_OSS AND (LMMS_HAVE_SOUNDCARD_H OR LMMS_HAVE_SYS_SOUNDCARD_H))
	SET(LMMS_HAVE_OSS TRUE)
	SET(STATUS_OSS "OK")
ELSEIF(WANT_OSS)
	SET(STATUS_OSS "<not found or not supported on this platform>")
ENDIF()


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
	IF(WANT_WEAKJACK)
		SET(LMMS_HAVE_WEAKJACK TRUE)
		SET(STATUS_JACK "OK (weak linking enabled)")
		set(JACK_INCLUDE_DIRS "")
		set(JACK_LIBRARIES weakjack)
		SET(LMMS_HAVE_JACK TRUE)
		SET(LMMS_HAVE_JACK_PRENAME TRUE)
		SET(JACK_FOUND TRUE)
	ELSE()
		PKG_CHECK_MODULES(JACK jack>=0.77)
		IF(JACK_FOUND)
			SET(LMMS_HAVE_JACK TRUE)
			SET(STATUS_JACK "OK")
			SET(CMAKE_REQUIRED_LIBRARIES_BACKUP "${CMAKE_REQUIRED_LIBRARIES}")
			SET(CMAKE_REQUIRED_LIBRARIES "${JACK_LIBRARIES}")
			CHECK_LIBRARY_EXISTS(jack jack_port_rename "" LMMS_HAVE_JACK_PRENAME)
			SET(CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES_BACKUP}")
			UNSET(CMAKE_REQUIRED_LIBRARIES_BACKUP)
		ENDIF()
	ENDIF()

	IF(NOT JACK_FOUND)
		SET(JACK_INCLUDE_DIRS "")
		SET(STATUS_JACK "not found")
	ENDIF()
ENDIF(WANT_JACK)

# check for FFTW3F-library
FIND_PACKAGE(FFTW COMPONENTS fftw3f REQUIRED)

# check for FLTK
set(FLTK_SKIP_OPENGL TRUE)
set(FLTK_SKIP_FORMS TRUE)
set(FLTK_SKIP_IMAGES TRUE)
set(FLTK_SKIP_MATH TRUE)
if(MINGW_PREFIX)
	set(FLTK_SKIP_FLUID TRUE)
endif()
FIND_PACKAGE(FLTK)
IF(FLTK_FOUND)
	SET(STATUS_ZYN "OK")
ELSE()
	SET(STATUS_ZYN "not found, please install fltk")
ENDIF()

# check for Fluidsynth
IF(WANT_SF2)
	find_package(FluidSynth 1.1.7)
	if(FluidSynth_FOUND)
		SET(LMMS_HAVE_FLUIDSYNTH TRUE)
		if(FluidSynth_VERSION_STRING VERSION_GREATER_EQUAL 2)
			set(STATUS_FLUIDSYNTH "OK")
		else()
			set(STATUS_FLUIDSYNTH "OK (FluidSynth version < 2: per-note panning unsupported)")
		endif()
	else()
		SET(STATUS_FLUIDSYNTH "not found, libfluidsynth-dev (or similar)"
			"is highly recommended")
	endif()
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
IF(LMMS_BUILD_LINUX OR LMMS_BUILD_APPLE OR LMMS_BUILD_OPENBSD OR LMMS_BUILD_FREEBSD OR LMMS_BUILD_HAIKU)
	FIND_PACKAGE(Threads)
ENDIF(LMMS_BUILD_LINUX OR LMMS_BUILD_APPLE OR LMMS_BUILD_OPENBSD OR LMMS_BUILD_FREEBSD OR LMMS_BUILD_HAIKU)

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
if(WANT_VST)
	if((WANT_VST_32 OR WANT_VST_64) AND NOT LMMS_BUILD_WIN32)
		find_package(Wine)
		include(CheckWineGcc)
	endif()
	macro(check_vst bits)
		if(NOT WANT_VST_${bits})
			set(STATUS_VST_${bits} "Not built, as requested")
		elseif(LMMS_BUILD_WIN32)
			set(STATUS_VST_${bits} "OK")
			set(LMMS_HAVE_VST_${bits} TRUE)
		elseif(NOT WINE_FOUND)
			set(STATUS_VST_${bits} "not found, please install (lib)wine-dev (or similar) - 64 bit systems additionally need gcc-multilib and g++-multilib")
		else()
			CheckWineGcc("${bits}" "${WINEGCC}" WINEGCC_WORKING)
			if(WINEGCC_WORKING)
				set(LMMS_HAVE_VST_${bits} TRUE)
				if(WINE_LIBRARY_FIX)
					set(STATUS_VST_${bits} "OK, with workaround linking ${WINE_LIBRARY_FIX}")
				else()
					set(STATUS_VST_${bits} "OK")
				endif()
			else()
				set(STATUS_VST_${bits} "winegcc fails to compile ${bits}-bit binaries, please make sure you have ${bits}-bit GCC libraries")
			endif()
		endif()
	endmacro()
	check_vst(32)
	check_vst(64)
	if(LMMS_HAVE_VST_32 OR LMMS_HAVE_VST_64 OR LMMS_BUILD_LINUX)
		set(LMMS_HAVE_VST TRUE)
		set(STATUS_VST "OK")
	else()
		set(STATUS_VST "No hosts selected and available")
	endif()
endif()

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

if(WANT_DEBUG_CPACK)
	if((LMMS_BUILD_WIN32 AND CMAKE_VERSION VERSION_LESS "3.19") OR WANT_CPACK_TARBALL)
		set(STATUS_DEBUG_CPACK "Wanted but disabled due to unsupported configuration")
	else()
		set(CPACK_DEBUG TRUE)
		set(STATUS_DEBUG_CPACK "Enabled")
	endif()
else()
	set(STATUS_DEBUG_CPACK "Disabled")
endif()

# check for libsamplerate
FIND_PACKAGE(Samplerate 0.1.8 MODULE REQUIRED)

IF(NOT CMAKE_BUILD_TYPE)
	message(STATUS "Setting build type to 'Release' as none was specified.")
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
	# Set the possible values of build type for cmake-gui
	SET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
		"MinSizeRel" "RelWithDebInfo")
ENDIF()

SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DLMMS_DEBUG")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DLMMS_DEBUG")

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(NDEBUG)
	SET(STATUS_ASSERTIONS "Disabled")
else()
    remove_definitions(-DNDEBUG)
	SET(STATUS_ASSERTIONS "Enabled")
endif()

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
	set(NOOP_COMMAND "${CMAKE_COMMAND}" "-E" "true")
else()
	set(NOOP_COMMAND "${CMAKE_COMMAND}" "-E" "echo")
endif()

if(STRIP)
	# TODO CMake 3.19: Now that CONFIG generator expressions support testing for
	# multiple configurations, combine the OR into a single CONFIG expression.
	set(STRIP_COMMAND "$<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>,${NOOP_COMMAND},${STRIP}>")
else()
	set(STRIP_COMMAND "${NOOP_COMMAND}")
endif()

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

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT WIN32)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -DPIC")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -DPIC")
elseif(MSVC)
	# Use UTF-8 as the source and execution character set
	add_compile_options("/utf-8")
ENDIF()

# gcc builds support gprof for profiling
# clang too seems to support gprof, but i couldn't get it working.
# if needed, change the if condition to "GNU|CLANG"
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	set(STATUS_GPROF ", NOT SUPPORTED BY THIS COMPILER")
	set(WANT_DEBUG_GPROF OFF)
endif()

if(WANT_DEBUG_GPROF)
	add_compile_options(-pg)
	add_link_options(-pg)
	set(STATUS_GPROF "OK")
else()
	set(STATUS_GPROF "Disabled ${STATUS_GPROF}")
endif()

# add enabled sanitizers
function(add_sanitizer sanitizer supported_compilers want_flag status_flag)
	if(${want_flag})
		if(CMAKE_CXX_COMPILER_ID MATCHES "${supported_compilers}")
			set("${status_flag}" "Enabled" PARENT_SCOPE)
			string(REPLACE ";" " " additional_flags "${ARGN}")
			# todo CMake 3.13: use add_compile_options/add_link_options instead
			set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=${sanitizer} ${additional_flags}" PARENT_SCOPE)
			set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=${sanitizer} ${additional_flags}" PARENT_SCOPE)
		else()
			set("${status_flag}" "Wanted but disabled due to unsupported compiler" PARENT_SCOPE)
		endif()
	else()
		set("${status_flag}" "Disabled" PARENT_SCOPE)
	endif()
endfunction()

add_sanitizer(address "GNU|Clang|MSVC" WANT_DEBUG_ASAN STATUS_DEBUG_ASAN)
add_sanitizer(thread "GNU|Clang" WANT_DEBUG_TSAN STATUS_DEBUG_TSAN)
add_sanitizer(memory "Clang" WANT_DEBUG_MSAN STATUS_DEBUG_MSAN -fno-omit-frame-pointer)
# UBSan does not link with vptr enabled due to a problem with references from PeakControllerEffect
# not being found by PeakController
add_sanitizer(undefined "GNU|Clang" WANT_DEBUG_UBSAN STATUS_DEBUG_UBSAN -fno-sanitize=vptr)

# Add warning and error flags
include(ErrorFlags)

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

FIND_PACKAGE(UnixCommands)
IF(GZIP)
	ADD_CUSTOM_COMMAND(OUTPUT "${CMAKE_BINARY_DIR}/lmms.1.gz"
					COMMAND ${GZIP} -c ${CMAKE_SOURCE_DIR}/doc/lmms.1 > ${CMAKE_BINARY_DIR}/lmms.1.gz
					DEPENDS "${CMAKE_SOURCE_DIR}/doc/lmms.1"
					COMMENT "Generating lmms.1.gz"
					VERBATIM)


	ADD_CUSTOM_TARGET(manpage ALL
					DEPENDS "${CMAKE_BINARY_DIR}/lmms.1.gz")
ELSEIF(UNIX)
	MESSAGE(FATAL_ERROR "Can't find gzip required for generating lmms.1.gz")
ENDIF()

# install headers

IF(LMMS_BUILD_LINUX)
	INSTALL(FILES ${LMMS_INCLUDES}
		"${CMAKE_BINARY_DIR}/lmmsconfig.h"
		"${CMAKE_BINARY_DIR}/lmmsversion.h"
		"${CMAKE_SOURCE_DIR}/src/gui/embed.cpp"
		DESTINATION "include/lmms/")
ENDIF(LMMS_BUILD_LINUX)

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
"* Lv2 plugins                 : ${STATUS_LV2}\n"
"* SUIL for plugin UIs         : ${STATUS_SUIL}\n"
"* ZynAddSubFX instrument      : ${STATUS_ZYN}\n"
"* Carla Patchbay & Rack       : ${STATUS_CARLA}\n"
"* SoundFont2 player           : ${STATUS_FLUIDSYNTH}\n"
"* Sid instrument              : ${STATUS_SID}\n"
"* Stk Mallets                 : ${STATUS_STK}\n"
"* VST plugin host             : ${STATUS_VST}\n"
"  * 32-bit Windows host       : ${STATUS_VST_32}\n"
"  * 64-bit Windows host       : ${STATUS_VST_64}\n"
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
"* Debug FP exceptions               : ${STATUS_DEBUG_FPE}\n"
"* Debug using AddressSanitizer      : ${STATUS_DEBUG_ASAN}\n"
"* Debug using ThreadSanitizer       : ${STATUS_DEBUG_TSAN}\n"
"* Debug using MemorySanitizer       : ${STATUS_DEBUG_MSAN}\n"
"* Debug using UBSanitizer           : ${STATUS_DEBUG_UBSAN}\n"
"* Debug packaging commands          : ${STATUS_DEBUG_CPACK}\n"
"* Profile using GNU profiler        : ${STATUS_GPROF}\n"
"* Debug assertions                  : ${STATUS_ASSERTIONS}\n"
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
