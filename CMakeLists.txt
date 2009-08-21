CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)

PROJECT(lmms)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

IF(COMMAND CMAKE_POLICY)
	CMAKE_POLICY(SET CMP0005 NEW)
	CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

INCLUDE(AddFileDependencies)
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckLibraryExists)
INCLUDE(FindPkgConfig)

SET(VERSION_MAJOR "0")
SET(VERSION_MINOR "4")
SET(VERSION_PATCH "90")
#SET(VERSION_SUFFIX "")
IF(NOT VERSION_SUFFIX)
	IF(EXISTS ${CMAKE_SOURCE_DIR}/.svn)
		INCLUDE(FindSubversion)
		IF(Subversion_FOUND)
			SET(ORIG_LANG "$ENV{LANG}")
			SET(ENV{LANG} "C")
			EXECUTE_PROCESS(WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
					COMMAND ${Subversion_SVN_EXECUTABLE} info
					COMMAND grep "^Revision:"
					OUTPUT_VARIABLE SVN_REV)
			SET(ENV{LANG} "${ORIG_LANG}")
			STRING(REGEX MATCH "[0-9]+" VERSION_SUFFIX "${SVN_REV}")
		ENDIF(Subversion_FOUND)
	ENDIF(EXISTS ${CMAKE_SOURCE_DIR}/.svn)
ENDIF(NOT VERSION_SUFFIX)

SET(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
IF(VERSION_SUFFIX)
	SET(VERSION "${VERSION}-${VERSION_SUFFIX}")
ENDIF(VERSION_SUFFIX)


INCLUDE(DetectMachine)

IF(LMMS_HOST_X86_64)
	SET(STRIP /opt/mingw64/bin/x86_64-pc-mingw32-strip)
ELSE(LMMS_HOST_X86_64)
	SET(STRIP /opt/mingw32/bin/i586-pc-mingw32-strip)
ENDIF(LMMS_HOST_X86_64)

OPTION(WANT_ALSA	"Include ALSA (Advanced Linux Sound Architecture) support" ON)
OPTION(WANT_CALF	"Include CALF LADSPA plugins" ON)
OPTION(WANT_CAPS	"Include C* Audio Plugin Suite (LADSPA plugins)" ON)
OPTION(WANT_CMT		"Include Computer Music Toolkit LADSPA plugins" ON)
OPTION(WANT_FFTW3F	"Include SpectrumAnalyzer and ZynAddSubFX plugin" ON)
OPTION(WANT_JACK	"Include JACK (Jack Audio Connection Kit) support" ON)
OPTION(WANT_LV2		"Include LV2 support" ON)
OPTION(WANT_OGGVORBIS	"Include OGG/Vorbis support" ON)
OPTION(WANT_PULSEAUDIO	"Include PulseAudio support" ON)
#OPTION(WANT_PORTAUDIO	"Include PortAudio support" ON)	# TODO: fix PortAudio support
OPTION(WANT_SDL		"Include SDL (Simple DirectMedia Layer) support" ON)
OPTION(WANT_SF2		"Include SoundFont2 player plugin" ON)
OPTION(WANT_STK		"Include Stk (Synthesis Toolkit) support" ON)
OPTION(WANT_SYSTEM_SR	"Use system's libsamplerate" ON)
OPTION(WANT_SWH		"Include Steve Harris's LADSPA plugins" ON)
OPTION(WANT_TAP		"Include Tom's Audio Processing LADSPA plugins" ON)
OPTION(WANT_VST		"Include VST support" ON)
OPTION(WANT_VST_NOWINE	"Include partial VST support (without wine)" OFF)
OPTION(WANT_WINMM	"Include WinMM MIDI support" OFF)
OPTION(WANT_ZIP		"Include zip support" ON)
OPTION(WANT_FLAC	"Include flac support" ON)

IF(LMMS_BUILD_WIN32)
	SET(WANT_ALSA OFF)
	SET(WANT_JACK OFF)
	SET(WANT_PULSEAUDIO OFF)
	SET(WANT_SYSTEM_SR OFF)
	SET(WANT_WINMM ON)
	SET(LMMS_HAVE_WINMM TRUE)
	SET(STATUS_ALSA "<not supported on this platform>")
	SET(STATUS_JACK "<not supported on this platform>")
	SET(STATUS_PULSEAUDIO "<not supported on this platform>")
	SET(STATUS_WINMM "OK")
ELSE(LMMS_BUILD_WIN32)
	SET(STATUS_WINMM "<not supported on this platform>")
ENDIF(LMMS_BUILD_WIN32)



CHECK_INCLUDE_FILES(stdint.h LMMS_HAVE_STDINT_H)
CHECK_INCLUDE_FILES(stdbool.h LMMS_HAVE_STDBOOL_H)
CHECK_INCLUDE_FILES(stdlib.h LMMS_HAVE_STDLIB_H)
CHECK_INCLUDE_FILES(pthread.h LMMS_HAVE_PTHREAD_H)
CHECK_INCLUDE_FILES(semaphore.h LMMS_HAVE_SEMAPHORE_H)
CHECK_INCLUDE_FILES(unistd.h LMMS_HAVE_UNISTD_H)
CHECK_INCLUDE_FILES(sys/types.h LMMS_HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILES(sys/ipc.h LMMS_HAVE_SYS_IPC_H)
CHECK_INCLUDE_FILES(sys/shm.h LMMS_HAVE_SYS_SHM_H)
CHECK_INCLUDE_FILES(sys/time.h LMMS_HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILES(sys/wait.h LMMS_HAVE_SYS_WAIT_H)
CHECK_INCLUDE_FILES(sys/select.h LMMS_HAVE_SYS_SELECT_H)
CHECK_INCLUDE_FILES(stdarg.h LMMS_HAVE_STDARG_H)
CHECK_INCLUDE_FILES(signal.h LMMS_HAVE_SIGNAL_H)
CHECK_INCLUDE_FILES(sched.h LMMS_HAVE_SCHED_H)
CHECK_INCLUDE_FILES(sys/soundcard.h LMMS_HAVE_SYS_SOUNDCARD_H)
CHECK_INCLUDE_FILES(soundcard.h LMMS_HAVE_SOUNDCARD_H)
CHECK_INCLUDE_FILES(fcntl.h LMMS_HAVE_FCNTL_H)
CHECK_INCLUDE_FILES(sys/ioctl.h LMMS_HAVE_SYS_IOCTL_H)
CHECK_INCLUDE_FILES(ctype.h LMMS_HAVE_CTYPE_H)
CHECK_INCLUDE_FILES(string.h LMMS_HAVE_STRING_H)
CHECK_INCLUDE_FILES(process.h LMMS_HAVE_PROCESS_H)
CHECK_INCLUDE_FILES(locale.h LMMS_HAVE_LOCALE_H)

# check for Qt4
SET(QT_MIN_VERSION "4.4.0")
FIND_PACKAGE(Qt4 REQUIRED)
SET(QT_USE_QTXML 1)
SET(QT_USE_QTNETWORK 1)
EXEC_PROGRAM(${QT_QMAKE_EXECUTABLE} ARGS "-query QT_INSTALL_TRANSLATIONS" OUTPUT_VARIABLE QT_TRANSLATIONS_DIR)
IF(EXISTS "${QT_TRANSLATIONS_DIR}")
	MESSAGE("-- Found Qt translations in ${QT_TRANSLATIONS_DIR}")
	ADD_DEFINITIONS(-D'QT_TRANSLATIONS_DIR="${QT_TRANSLATIONS_DIR}"')
ENDIF(EXISTS "${QT_TRANSLATIONS_DIR}")
IF(NOT WIN32)
	STRING(REPLACE "-DQT_DLL" "" QT_DEFINITIONS "${QT_DEFINITIONS}")
ENDIF(NOT WIN32)
INCLUDE(${QT_USE_FILE})

# check for libsndfile
PKG_CHECK_MODULES(SNDFILE REQUIRED sndfile>=1.0.11)
IF(NOT SNDFILE_FOUND)
	MESSAGE(FATAL_ERROR "LMMS requires libsndfile1 and libsndfile1-dev >= 1.0.11 - please install, remove CMakeCache.txt and try again!")
ENDIF(NOT SNDFILE_FOUND)

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


# check for SDL
IF(WANT_SDL)
	SET(SDL_BUILDING_LIBRARY TRUE)
	FIND_PACKAGE(SDL)
	IF(SDL_FOUND)
		SET(LMMS_HAVE_SDL TRUE)
		SET(STATUS_SDL "OK")
	ELSE(SDL_FOUND)
		SET(STATUS_SDL "not found, please install libsdl1.2-dev (or similiar) "
			"if you require SDL support")
	ENDIF(SDL_FOUND)
ENDIF(WANT_SDL)
IF(NOT LMMS_HAVE_SDL)
	SET(SDL_INCLUDE_DIR "")
ELSE(NOT LMMS_HAVE_SDL)
	IF(NOT SDL_INCLUDE_DIR)
		SET(SDL_INCLUDE_DIR ${CMAKE_FIND_ROOT_PATH}/include)
	ENDIF(NOT SDL_INCLUDE_DIR)
ENDIF(NOT LMMS_HAVE_SDL)

# check for libzip
IF(WANT_ZIP)
	FIND_PACKAGE(ZIP)
	IF(ZIP_FOUND)
		SET(LMMS_HAVE_ZIP TRUE)
		SET(STATUS_ZIP "OK")
	ELSE(ZIP_FOUND)
		SET(STATUS_ZIP "not found, install libzip-dev (or similar) "
			"if you want to import FL .zip files")
	ENDIF(ZIP_FOUND)
ENDIF(WANT_ZIP)

# check for libflac++
IF(WANT_FLAC)
    FIND_PACKAGE(FLAC)
    IF(FLAC_FOUND)
        SET(LMMS_HAVE_FLAC TRUE)
        SET(STATUS_FLAC "OK")
    ELSE(FLAC_FOUND)
        SET(STATUS_FLAC "not found, install libflac++-dev (or similar) "
            "if you want flac as an export format")
    ENDIF(FLAC_FOUND)
ENDIF(WANT_FLAC)

# check for Stk
IF(WANT_STK)
	FIND_PACKAGE(STK)
	IF(STK_FOUND)
		SET(LMMS_HAVE_STK TRUE)
		SET(STATUS_STK "OK")
	ELSE(STK_FOUND)
		SET(STK_INCLUDE_DIR "")
		SET(STATUS_STK "not found, please install libstk0-dev (or similiar) "
			"if you require the Vibed Instrument")
	ENDIF(STK_FOUND)
ENDIF(WANT_STK)


# check for PortAudio
#IF(WANT_PORTAUDIO)
#	FIND_PACKAGE(Portaudio)
#	IF(PORTAUDIO_FOUND)
#		SET(LMMS_HAVE_PORTAUDIO TRUE)
#		SET(STATUS_PORTAUDIO "OK")
#	ELSE(PORTAUDIO_FOUND)
#		SET(STATUS_PORTAUDIO "not found, please install libportaudio-dev (or similiar, version >= 1.8) "
#			"if you require Portaudio support")
#	ENDIF(PORTAUDIO_FOUND)
#ENDIF(WANT_PORTAUDIO)
IF(NOT LMMS_HAVE_PORTAUDIO)
	SET(PORTAUDIO_INCLUDE_DIR "")
	SET(PORTAUDIO_LIBRARIES "")
ENDIF(NOT LMMS_HAVE_PORTAUDIO)


# check for PulseAudio
IF(WANT_PULSEAUDIO)
	FIND_PACKAGE(PulseAudio)
	IF(PULSEAUDIO_FOUND)
		SET(LMMS_HAVE_PULSEAUDIO TRUE)
		SET(STATUS_PULSEAUDIO "OK")
	ELSE(PULSEAUDIO_FOUND)
		SET(STATUS_PULSEAUDIO "not found, please install libpulse-dev (or similiar) "
			"if you require PulseAudio support")
	ENDIF(PULSEAUDIO_FOUND)
ENDIF(WANT_PULSEAUDIO)
IF(NOT LMMS_HAVE_PULSEAUDIO)
	SET(PULSEAUDIO_INCLUDE_DIR "")
	SET(PULSEAUDIO_LIBRARIES "")
ENDIF(NOT LMMS_HAVE_PULSEAUDIO)


# check for OGG/Vorbis-libraries
IF(WANT_OGGVORBIS)
	FIND_PACKAGE(OggVorbis)
	IF(OGGVORBIS_FOUND)
		SET(LMMS_HAVE_OGGVORBIS TRUE)
		SET(STATUS_OGGVORBIS "OK")
	ELSE(OGGVORBIS_FOUND)
		SET(STATUS_OGGVORBIS "not found, libogg-dev and libvorbis-dev (or similiar) "
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
		SET(STATUS_ALSA "not found, please install libasound2-dev (or similiar) "
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
		SET(LMMS_HAVE_JACK TRUE)
		SET(STATUS_JACK "OK")
	ELSE(JACK_FOUND)
		SET(STATUS_JACK "not found, please install libjack0.100.0-dev (or similiar) "
			"if you require JACK support")
	ENDIF(JACK_FOUND)
ENDIF(WANT_JACK)


# check for FFTW3F-library
IF(WANT_FFTW3F)
	PKG_CHECK_MODULES(FFTW3F fftw3f>=3.0.0)
	IF(FFTW3F_FOUND)
		SET(LMMS_HAVE_FFTW3F TRUE)
		SET(STATUS_FFTW3F "OK")
	ELSE(FFTW3F_FOUND)
		SET(STATUS_FFTW3F "not found, libfftw3-dev (or similiar) "
			"is highly recommended")
	ENDIF(FFTW3F_FOUND)
ENDIF(WANT_FFTW3F)


# check for Fluidsynth
IF(WANT_SF2)
	PKG_CHECK_MODULES(FLUIDSYNTH fluidsynth>=1.0.7)
	IF(FLUIDSYNTH_FOUND)
		SET(LMMS_HAVE_FLUIDSYNTH TRUE)
		SET(STATUS_FLUIDSYNTH "OK")
	ELSE(FLUIDSYNTH_FOUND)
		SET(STATUS_FLUIDSYNTH "not found, libfluidsynth-dev (or similiar)"
			"is highly recommended")
	ENDIF(FLUIDSYNTH_FOUND)
ENDIF(WANT_SF2)

# check for LV2 (this has an implicit dependency on RDF as far as I can tell)
IF(WANT_LV2)
	CHECK_INCLUDE_FILES(lv2.h LMMS_HAVE_LV2CORE)
	# compatibility for slv2 <= 0.6.2
	CHECK_INCLUDE_FILES(slv2/scalepoints.h LMMS_HAVE_SLV2_SCALEPOINTS_H)

	CHECK_LIBRARY_EXISTS(slv2 slv2_world_new "" LMMS_HAVE_SLV2)

# MESSAGE("SLV2 Exists    : '${LMMS_HAVE_SLV2}'")
# MESSAGE("SLV2 Library   : '${SLV2_LIBRARY}'")   #RIGHT
# MESSAGE("SLV2 Libraries : '${SLV2_LIBRARIES}'") # WRONG

	CHECK_LIBRARY_EXISTS(rdf librdf_new_world "" LMMS_HAVE_RDF)

###  TODO : mdda : Install requires :
# Fedora CCRMA : Want "lv2core"
# Fedora CCRMA : Want "slv2"

	SET(STATUS_LV2 "OK")

	IF(NOT LMMS_HAVE_LV2CORE)
		SET(STATUS_LV2 "LV2 devel packages not found : need lv2core(-devel)")
	ENDIF(NOT LMMS_HAVE_LV2CORE)

	IF(NOT LMMS_HAVE_SLV2)
		SET(STATUS_LV2 "LV2 (Host) devel packages not found : need libslv2-dev, slv2-devel or similiar")
	ENDIF(NOT LMMS_HAVE_SLV2)

	IF(NOT LMMS_HAVE_RDF)
		SET(STATUS_LV2 "LV2 (RDF) devel packages not found : need librdf0-dev, redland-devel or similiar")
	ENDIF(NOT LMMS_HAVE_RDF)

	IF("${STATUS_LV2}" STREQUAL "OK")
		FIND_LIBRARY(SLV2_LIBRARY NAMES slv2 PATH /usr/lib /usr/local/lib)
		FIND_LIBRARY(RDF_LIBRARY NAMES rdf PATH /usr/lib /usr/local/lib)
		SET(LMMS_SUPPORT_LV2 TRUE)
		SET(LMMS_HAVE_LV2 TRUE)
	 ENDIF("${STATUS_LV2}" STREQUAL "OK")
ENDIF(WANT_LV2)


# check for WINE
IF(WANT_VST)
	INCLUDE(CheckIncludeFileCXX)
	SET(CMAKE_REQUIRED_FLAGS_ORIG ${CMAKE_REQUIRED_FLAGS})
	SET(CMAKE_REQUIRED_INCLUDES_ORIG ${CMAKE_REQUIRED_INCLUDES})
	SET(CMAKE_CXX_COMPILER_ORIG ${CMAKE_CXX_COMPILER})
	IF(LMMS_HOST_X86_64)
		SET(CMAKE_REQUIRED_FLAGS -m32 ${CMAKE_REQUIRED_FLAGS})
	ENDIF(LMMS_HOST_X86_64)
	CHECK_LIBRARY_EXISTS(wine wine_init "" HAVE_LIBWINE)
	SET(CMAKE_CXX_COMPILER /usr/bin/wineg++)
	SET(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${CMAKE_INSTALL_PREFIX}/include/wine/windows /usr/include/wine/windows)
	CHECK_INCLUDE_FILE_CXX(windows.h HAVE_WINDOWS_H)
	SET(CMAKE_CXX_COMPILER ${CMAKE_CXX_COMPILER_ORIG})
	SET(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS_ORIG})
	SET(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES_ORIG})
	IF(HAVE_LIBWINE AND HAVE_WINDOWS_H)
		SET(LMMS_SUPPORT_VST TRUE)
		SET(STATUS_VST "OK")
	ELSE(HAVE_LIBWINE AND HAVE_WINDOWS_H)
		SET(STATUS_VST "not found, please install (lib)wine-dev (or similiar) - 64 bit systems additionally need gcc-multilib and g++-multilib")
	ENDIF(HAVE_LIBWINE AND HAVE_WINDOWS_H)
ENDIF(WANT_VST)
IF(LMMS_BUILD_WIN32)
	SET(LMMS_SUPPORT_VST TRUE)
	SET(STATUS_VST "OK")
ENDIF(LMMS_BUILD_WIN32)


# check for libsamplerate
IF(WANT_SYSTEM_SR)
	PKG_CHECK_MODULES(SAMPLERATE samplerate>=0.1.7)
	IF(SAMPLERATE_FOUND)
		SET(LMMS_HAVE_SAMPLERATE TRUE)
	ENDIF(SAMPLERATE_FOUND)
ENDIF(WANT_SYSTEM_SR)
IF(NOT LMMS_HAVE_SAMPLERATE)
	INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/3rdparty/samplerate)
	SET(CMAKE_CROSSCOMPILING_ORIG "${CMAKE_CROSSCOMPILING}")
	SET(CMAKE_CROSSCOMPILING "")
	INCLUDE(ChecksForLibSamplerate)
	SET(LIBSAMPLERATE_SOURCES 
		${CMAKE_SOURCE_DIR}/src/3rdparty/samplerate/samplerate.c
		${CMAKE_SOURCE_DIR}/src/3rdparty/samplerate/samplerate.h
		${CMAKE_SOURCE_DIR}/src/3rdparty/samplerate/src_sinc.c
		${CMAKE_SOURCE_DIR}/src/3rdparty/samplerate/high_qual_coeffs.h
		${CMAKE_SOURCE_DIR}/src/3rdparty/samplerate/mid_qual_coeffs.h
		${CMAKE_SOURCE_DIR}/src/3rdparty/samplerate/fastest_coeffs.h
		${CMAKE_SOURCE_DIR}/src/3rdparty/samplerate/common.h
		${CMAKE_SOURCE_DIR}/src/3rdparty/samplerate/config.h
		${CMAKE_SOURCE_DIR}/src/3rdparty/samplerate/float_cast.h
		${CMAKE_SOURCE_DIR}/src/3rdparty/samplerate/src_zoh.c
		${CMAKE_SOURCE_DIR}/src/3rdparty/samplerate/src_linear.c)
	SET(CMAKE_CROSSCOMPILING "${CMAKE_CROSSCOMPILING_ORIG}")
ENDIF(NOT LMMS_HAVE_SAMPLERATE)


CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/lmmsconfig.h.in ${CMAKE_BINARY_DIR}/lmmsconfig.h)
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/lmmsversion.h.in ${CMAKE_BINARY_DIR}/lmmsversion.h)
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/lmms.rc.in ${CMAKE_BINARY_DIR}/lmms.rc)

# set compiler flags
#SET(CMAKE_BUILD_TYPE relwithdebug)
SET(CMAKE_C_FLAGS "-O2 -g -Wall -ftree-vectorize ${CMAKE_C_FLAGS}")
SET(CMAKE_CXX_FLAGS "-O2 -g -fno-exceptions -Wall -ftree-vectorize ${CMAKE_CXX_FLAGS}")
#SET(CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE} "${CMAKE_C_FLAGS}")
#SET(CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE} "${CMAKE_CXX_FLAGS}")


FILE(GLOB_RECURSE lmms_INCLUDES ${CMAKE_SOURCE_DIR}/include/*.h)
FILE(GLOB lmms_UI ${CMAKE_SOURCE_DIR}/src/gui/Forms/*.ui)
FILE(GLOB_RECURSE lmms_SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)

SET(lmms_MOC ${lmms_INCLUDES})

SET(lmms_EMBEDDED_RESOURCES ${CMAKE_SOURCE_DIR}/AUTHORS ${CMAKE_SOURCE_DIR}/COPYING)

QT4_WRAP_CPP(lmms_MOC_out ${lmms_MOC})
QT4_WRAP_UI(lmms_UI_out ${lmms_UI})


# embedded resources stuff
IF(WIN32 OR WIN64)
	# compile buildtools native
	SET(BIN2RES_CPP ${CMAKE_SOURCE_DIR}/buildtools/bin2res.cpp)
	SET(BIN2RES ${CMAKE_BINARY_DIR}/bin2res)
	ADD_CUSTOM_COMMAND(OUTPUT ${BIN2RES} COMMAND g++ ARGS ${BIN2RES_CPP} -o ${BIN2RES} DEPENDS ${BIN2RES_CPP})
ELSE(WIN32 OR WIN64)
	ADD_EXECUTABLE(bin2res buildtools/bin2res.cpp)
	GET_TARGET_PROPERTY(BIN2RES bin2res LOCATION)
ENDIF(WIN32 OR WIN64)

SET(LMMS_ER_H ${CMAKE_CURRENT_BINARY_DIR}/embedded_resources.h)

# we somehow have to make LMMS-binary depend on MOC-files
ADD_FILE_DEPENDENCIES(${CMAKE_BINARY_DIR}/lmmsconfig.h ${lmms_MOC_out})

ADD_CUSTOM_COMMAND(OUTPUT ${LMMS_ER_H} COMMAND ${BIN2RES} ARGS ${lmms_EMBEDDED_RESOURCES} > ${LMMS_ER_H} DEPENDS ${BIN2RES})

# build CPU specific optimized modules
IF(LMMS_HOST_X86 OR LMMS_HOST_X86_64)

	IF(LMMS_HOST_X86)
		SET(opt_targets mmx sse sse2)
	ELSE(LMMS_HOST_X86)
		SET(opt_targets sse sse2)
	ENDIF(LMMS_HOST_X86)

FOREACH(opt_target ${opt_targets})

	STRING(TOUPPER ${opt_target} OPT_TARGET)

	SET(BASIC_OPS_X86_TARGET_S "")
	SET(CPU_X86_C "${CMAKE_SOURCE_DIR}/src/core/CpuX86.c")
	SET(CPU_X86_TARGET_O "${CMAKE_BINARY_DIR}/CpuX86_${opt_target}.o")
	SET(FPMATH_FLAGS "")
	IF(NOT "${OPT_TARGET}" STREQUAL "MMX")
		SET(FPMATH_FLAGS "-mfpmath=sse")
	ENDIF(NOT "${OPT_TARGET}" STREQUAL "MMX")
	IF("${CMAKE_C_COMPILER_ARG1}" STREQUAL " gcc")
		SET(CMAKE_C_COMPILER_ARG1 gcc)
	ENDIF("${CMAKE_C_COMPILER_ARG1}" STREQUAL " gcc")
	SET(COMPILE_CMD ${CMAKE_C_COMPILER} ${CPU_X86_C} -O2 -fno-stack-protector -ftree-vectorize -fomit-frame-pointer -c -I${CMAKE_SOURCE_DIR}/include -I${CMAKE_BINARY_DIR} -g0 -DBUILD_${OPT_TARGET} -m${opt_target} ${FPMATH_FLAGS})
	ADD_CUSTOM_COMMAND(OUTPUT ${CPU_X86_TARGET_O} COMMAND ${COMPILE_CMD} -o ${CPU_X86_TARGET_O} DEPENDS ${CPU_X86_C})
	ADD_CUSTOM_TARGET(debug-${opt_target} COMMAND ${COMPILE_CMD} -o ${CPU_X86_TARGET_O}.s -S -ftree-vectorizer-verbose=2)
	SET(cpu_objects ${cpu_objects} ${CPU_X86_TARGET_O})

ENDFOREACH(opt_target ${opt_targets})

	SET(lmms_SOURCES ${lmms_SOURCES} ${cpu_objects})

ENDIF(LMMS_HOST_X86 OR LMMS_HOST_X86_64)


IF(WIN32)
	SET(WINRC "${CMAKE_BINARY_DIR}/lmmsrc.obj")
	IF(LMMS_HOST_X86_64)
		SET(WINDRES /opt/mingw64/bin/x86_64-pc-mingw32-windres)
	ELSE(LMMS_HOST_X86_64)
		SET(WINDRES /opt/mingw32/bin/i586-pc-mingw32-windres)
	ENDIF(LMMS_HOST_X86_64)
	ADD_CUSTOM_COMMAND(OUTPUT ${WINRC}
				COMMAND ${WINDRES}
					-I${CMAKE_SOURCE_DIR}
					-o${CMAKE_BINARY_DIR}/lmmsrc.obj
					-i${CMAKE_BINARY_DIR}/lmms.rc
				DEPENDS ${CMAKE_BINARY_DIR}/lmms.rc)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes")
ELSE(WIN32)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -DPIC")
ENDIF(WIN32)



# make sub-directories
ADD_SUBDIRECTORY(plugins)
ADD_SUBDIRECTORY(data)


#
# build LMMS-binary
#
ADD_DEFINITIONS(-D'LIB_DIR="${CMAKE_INSTALL_PREFIX}/${LIB_DIR}/"' -D'PLUGIN_DIR="${CMAKE_INSTALL_PREFIX}/${LIB_DIR}/lmms/"' ${PULSEAUDIO_DEFINITIONS} ${PORTAUDIO_DEFINITIONS})

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR}/include ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include ${SDL_INCLUDE_DIR} ${PORTAUDIO_INCLUDE_DIR} ${PULSEAUDIO_INCLUDE_DIR} ${JACK_INCLUDE_DIRS} ${OGGVORBIS_INCLUDE_DIR} ${SAMPLERATE_INCLUDE_DIRS} ${SNDFILE_INCLUDE_DIRS} ${ZIP_INCLUDE_DIR} ${FLAC_INCLUDE_DIR})
LINK_DIRECTORIES(${CMAKE_INSTALL_PREFIX}/lib ${ASOUND_LIBRARY_DIR} ${JACK_LIBRARY_DIRS} ${SAMPLERATE_LIBRARY_DIRS} ${SNDFILE_LIBRARY_DIRS})
LINK_LIBRARIES(${QT_LIBRARIES} ${ASOUND_LIBRARY} ${SDL_LIBRARY} ${PORTAUDIO_LIBRARIES} ${PULSEAUDIO_LIBRARIES} ${JACK_LIBRARIES} ${OGGVORBIS_LIBRARIES} ${SAMPLERATE_LIBRARIES} ${SNDFILE_LIBRARIES} ${ZIP_LIBRARIES} ${FLAC_LIBRARIES} ${SLV2_LIBRARY})

ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_BINARY_DIR}/lmms.1.gz COMMAND gzip -c ${CMAKE_SOURCE_DIR}/lmms.1 > ${CMAKE_BINARY_DIR}/lmms.1.gz DEPENDS ${CMAKE_SOURCE_DIR}/lmms.1 COMMENT "Generating lmms.1.gz")

ADD_EXECUTABLE(lmms ${lmms_SOURCES} ${lmms_INCLUDES} ${LIBSAMPLERATE_SOURCES} ${LMMS_ER_H} ${lmms_UI_out} lmmsconfig.h lmmsversion.h ${WINRC} ${CMAKE_BINARY_DIR}/lmms.1.gz)

IF(LMMS_BUILD_WIN32)

	SET_TARGET_PROPERTIES(lmms PROPERTIES LINK_FLAGS "${LINK_FLAGS} -mwindows")
	ADD_CUSTOM_COMMAND(TARGET lmms POST_BUILD COMMAND ${STRIP} ${CMAKE_BINARY_DIR}/lmms.exe)

	INSTALL(TARGETS lmms RUNTIME DESTINATION .)
	IF(LMMS_BUILD_WIN64)
		INSTALL(FILES /opt/mingw64/bin/QtCore4.dll /opt/mingw64/bin/QtGui4.dll /opt/mingw64/bin/QtXml4.dll /opt/mingw64/bin/QtNetwork4.dll /opt/mingw64/bin/libsndfile-1.dll /opt/mingw64/bin/libvorbis-0.dll /opt/mingw64/bin/libvorbisenc-2.dll /opt/mingw64/bin/libvorbisfile-3.dll /opt/mingw64/bin/libogg-0.dll /opt/mingw64/bin/libfluidsynth-1.dll /opt/mingw64/bin/libfftw3f-3.dll /opt/mingw64/bin/SDL.dll DESTINATION .)
	ELSE(LMMS_BUILD_WIN64)
		INSTALL(FILES /opt/mingw32/bin/QtCore4.dll /opt/mingw32/bin/QtGui4.dll /opt/mingw32/bin/QtXml4.dll /opt/mingw32/bin/QtNetwork4.dll /opt/mingw32/bin/libz.dll /opt/mingw32/bin/libsndfile-1.dll /opt/mingw32/bin/libvorbis-0.dll /opt/mingw32/bin/libvorbisenc-2.dll /opt/mingw32/bin/libvorbisfile-3.dll /opt/mingw32/bin/libogg-0.dll /opt/mingw32/bin/libfluidsynth-1.dll /opt/mingw32/bin/libfftw3f-3.dll /opt/mingw32/bin/SDL.dll /opt/mingw32/i586-mingw32/bin/mingwm10.dll DESTINATION .)
	ENDIF(LMMS_BUILD_WIN64)

ELSE(LMMS_BUILD_WIN32)
	IF(NOT LMMS_BUILD_APPLE)
		SET_TARGET_PROPERTIES(lmms PROPERTIES LINK_FLAGS "${LINK_FLAGS} -Wl,-E")
	ENDIF(NOT LMMS_BUILD_APPLE)

	INSTALL(TARGETS lmms RUNTIME DESTINATION bin)
	INSTALL(FILES ${CMAKE_BINARY_DIR}/lmms.1.gz DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1/ PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)

ENDIF(LMMS_BUILD_WIN32)

#
# rules for building localizations
#
FILE(GLOB lmms_LOCALES data/locale/*.ts)
SET(ts_targets "")
SET(qm_targets "")
FOREACH(_ts_file ${lmms_LOCALES})
	STRING(REPLACE "${CMAKE_SOURCE_DIR}/data/locale/" "" _ts_target "${_ts_file}")
	STRING(REPLACE ".ts" ".qm" _qm_file "${_ts_file}")
	STRING(REPLACE ".ts" ".qm" _qm_target "${_ts_target}")
	ADD_CUSTOM_TARGET(${_ts_target} COMMAND ${QT_LUPDATE_EXECUTABLE} ${lmms_SOURCES} `find ${CMAKE_SOURCE_DIR}/plugins/ -type f -name '*.cpp'` -ts ${_ts_file})
	ADD_CUSTOM_TARGET(${_qm_target} COMMAND ${QT_LRELEASE_EXECUTABLE} ${_ts_file} -qm ${_qm_file})
	LIST(APPEND ts_targets "${_ts_target}")
	LIST(APPEND qm_targets "${_qm_target}")
ENDFOREACH(_ts_file ${lmms_LOCALES})

ADD_CUSTOM_TARGET(update-locales)
FOREACH(_item ${ts_targets})
	ADD_DEPENDENCIES(update-locales ${_item})
ENDFOREACH(_item ${ts_targets})

ADD_CUSTOM_TARGET(finalize-locales)
FOREACH(_item ${qm_targets})
	ADD_DEPENDENCIES(finalize-locales ${_item})
ENDFOREACH(_item ${qm_targets})


# install headers

IF(LMMS_BUILD_LINUX)
INSTALL(FILES ${lmms_INCLUDES} ${CMAKE_BINARY_DIR}/lmmsconfig.h ${CMAKE_BINARY_DIR}/lmmsversion.h ${CMAKE_SOURCE_DIR}/src/gui/embed.cpp DESTINATION ${CMAKE_INSTALL_PREFIX}/include/lmms/)
IF(NOT LMMS_HAVE_SAMPLERATE)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/src/3rdparty/samplerate/samplerate.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include/lmms/)
ENDIF(NOT LMMS_HAVE_SAMPLERATE)
ENDIF(LMMS_BUILD_LINUX)

# package ZynAddSubFX into win32 build
IF(LMMS_BUILD_WIN32)
IF(EXISTS ${CMAKE_SOURCE_DIR}/extras)
ADD_SUBDIRECTORY(${CMAKE_SOURCE_DIR}/extras/data/presets)
FILE(GLOB ZASF_BINARIES ${CMAKE_SOURCE_DIR}/extras/plugins/zynaddsubfx/zynaddsubfx.dll ${CMAKE_SOURCE_DIR}/extras/plugins/zynaddsubfx/remote_zynaddsubfx.exe)
INSTALL(FILES ${ZASF_BINARIES} DESTINATION ${PLUGIN_DIR})
ENDIF(EXISTS ${CMAKE_SOURCE_DIR}/extras)
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
			COMMAND rm -rf ${TMP}
			COMMAND mkdir -p ${TMP}
			COMMAND cp AUTHORS build_mingw32 ChangeLog CMakeLists.txt configure COPYING INSTALL lmms.1 lmms.rc.in lmms.spec.in lmmsconfig.h.in README TODO ${TMP}
			COMMAND cp -r buildtools cmake data include plugins src ${TMP}
			COMMAND rm -rf `find ${TMP} -type d -name ".svn"`  `find ${TMP} -name cmake_install.cmake` `find ${TMP} -name Makefile` `find ${TMP} -type d -name CMakeFiles` ${TMP}/CMakeCache.txt
			COMMAND tar cjf lmms-${VERSION}.tar.bz2 ${TMP}
			COMMAND rm -rf ${TMP})

#
# add win32-pkg-target (deprecated - use "package" target instead)
#
ADD_CUSTOM_TARGET(win32-pkg
			COMMAND mkdir -p tmp/lmms/data
			COMMAND mkdir -p tmp/lmms/plugins/ladspa/
			COMMAND cp lmms.exe tmp/lmms
			COMMAND find plugins/ -maxdepth 2 -name '*.dll'  -exec cp '{}' tmp/lmms/plugins/ "';'"
			COMMAND rm tmp/lmms/plugins/caps.dll
			COMMAND rm tmp/lmms/plugins/tap*.dll
			COMMAND cp plugins/ladspa_effect/caps/caps.dll tmp/lmms/plugins/ladspa/
			COMMAND cp plugins/ladspa_effect/tap/tap*.dll tmp/lmms/plugins/ladspa/
			COMMAND cd data && make DESTDIR=${CMAKE_BINARY_DIR}/tmp/lmms/ install
			COMMAND mv tmp/lmms/opt/mingw32/share/lmms/* tmp/lmms/data/ && rm -rf tmp/lmms/opt
			COMMAND cp /opt/mingw32/bin/QtCore4.dll tmp/lmms
			COMMAND cp /opt/mingw32/bin/QtGui4.dll tmp/lmms
			COMMAND cp /opt/mingw32/bin/QtXml4.dll tmp/lmms
			COMMAND cp /opt/mingw32/bin/QtNetwork4.dll tmp/lmms
			COMMAND cp /opt/mingw32/bin/libz.dll tmp/lmms
			COMMAND cp /opt/mingw32/bin/libsndfile-1.dll tmp/lmms
			COMMAND cp /opt/mingw32/bin/libvorbis*.dll tmp/lmms
			COMMAND cp /opt/mingw32/bin/libogg-0.dll tmp/lmms
			COMMAND cp /opt/mingw32/bin/libfluidsynth-1.dll tmp/lmms
			COMMAND cp /opt/mingw32/bin/libfftw3f-3.dll tmp/lmms
			COMMAND cp /opt/mingw32/bin/SDL.dll tmp/lmms
			COMMAND cp /opt/mingw32/i586-pc-mingw32/bin/mingwm10.dll tmp/lmms
			COMMAND cp -L ${CMAKE_SOURCE_DIR}/COPYING tmp/lmms/LICENSE.TXT
			COMMAND cp -L ${CMAKE_SOURCE_DIR}/README tmp/lmms/README.TXT
			COMMAND /opt/mingw32/bin/i586-pc-mingw32-strip tmp/lmms/lmms.exe tmp/lmms/plugins/*.dll tmp/lmms/plugins/ladspa/*.dll
			COMMAND mv tmp/lmms tmp/lmms-${VERSION}
			COMMAND cd tmp && zip -r -9 ../lmms-${VERSION}-bin-win32.zip lmms-${VERSION}/*
			COMMAND rm -rf tmp
)


SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${LMMS_ER_H} ${lmms_MOC_out} ${lmms_UI_out} lmmsconfig.h lmms.1.gz")


#
# display configuration information
#

MESSAGE("\n"
"Installation Summary\n"
"--------------------\n"
"* Install Directory           : ${CMAKE_INSTALL_PREFIX}\n"
#"* Build type                  : ${CMAKE_BUILD_TYPE}\n"
"* Use system's libsamplerate  : ${LMMS_HAVE_SAMPLERATE}\n"
)

MESSAGE(
"Supported audio interfaces\n"
"--------------------------\n"
"* ALSA                        : ${STATUS_ALSA}\n"
"* JACK                        : ${STATUS_JACK}\n"
"* OSS                         : ${STATUS_OSS}\n"
#"* PortAudio                   : ${STATUS_PORTAUDIO}\n"
"* PulseAudio                  : ${STATUS_PULSEAUDIO}\n"
"* SDL                         : ${STATUS_SDL}\n"
)

MESSAGE(
"Supported MIDI interfaces\n"
"-------------------------\n"
"* ALSA                        : ${STATUS_ALSA}\n"
"* OSS                         : ${STATUS_OSS}\n"
"* WinMM                       : ${STATUS_WINMM}\n"
)

MESSAGE(
"Supported file formats for project export\n"
"-----------------------------------------\n"
"* WAVE                        : OK\n"
"* OGG/VORBIS                  : ${STATUS_OGGVORBIS}\n"
"* FLAC                        : ${STATUS_FLAC}\n"
)

MESSAGE(
"Optional plugins\n"
"----------------\n"
"* SoundFont2 player           : ${STATUS_FLUIDSYNTH}\n"
"* Stk Mallets                 : ${STATUS_STK}\n"
"* VST-instrument hoster       : ${STATUS_VST}\n"
"* VST-effect hoster           : ${STATUS_VST}\n"
"* LV2 hoster                  : ${STATUS_LV2}\n"
"* SpectrumAnalyzer            : ${STATUS_FFTW3F}\n"
"* CALF LADSPA plugins         : ${STATUS_CALF}\n"
"* CAPS LADSPA plugins         : ${STATUS_CAPS}\n"
"* CMT LADSPA plugins          : ${STATUS_CMT}\n"
"* TAP LADSPA plugins          : ${STATUS_TAP}\n"
"* SWH LADSPA plugins          : ${STATUS_SWH}\n"
"* FL .zip import              : ${STATUS_ZIP}\n"
"* ZynAddSubFX                 : ${STATUS_FFTW3F}\n"
)

MESSAGE(
"\n"
"-----------------------------------------------------------------\n"
"IMPORTANT:\n"
"after installing missing packages, remove CMakeCache.txt before\n"
"running cmake again!\n"
"-----------------------------------------------------------------\n"
"\n\n")

INCLUDE(InstallRequiredSystemLibraries)
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Linux MultiMedia Studio - easy music production for everyone!")
SET(CPACK_PACKAGE_VENDOR "LMMS Developers")
IF(LMMS_BUILD_APPLE)
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/README ${CMAKE_BINARY_DIR}/README.txt COPYONLY)
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/COPYING ${CMAKE_BINARY_DIR}/COPYING.txt COPYONLY)
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_BINARY_DIR}/README.txt")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_BINARY_DIR}/COPYING.txt")
ELSE(LMMS_BUILD_APPLE)
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
ENDIF(LMMS_BUILD_APPLE)

SET(CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}")
IF(VERSION_SUFFIX)
	SET(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}-${VERSION_SUFFIX}")
ENDIF(VERSION_SUFFIX)
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "LMMS ${VERSION}")
IF(WIN32)
SET(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/data\\\\nsis_branding.bmp")
SET(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/data\\\\lmms.ico")
SET(CPACK_NSIS_INSTALLED_ICON_NAME "lmms.exe")
SET(CPACK_NSIS_DISPLAY_NAME "Linux MultiMedia Studio (LMMS)")
SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\lmms.sourceforge.net")
SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\lmms.sourceforge.net")
SET(CPACK_NSIS_CONTACT "lmms-devel@lists.sourceforge.net")
SET(CPACK_PACKAGE_EXECUTABLES "lmms.exe;Linux MultiMedia Studio")
SET(CPACK_NSIS_MENU_LINKS "lmms.exe;Linux MultiMedia Studio")
ELSE(WIN32)
SET(CPACK_STRIP_FILES "bin/lmms;${PLUGIN_DIR}/*.so")
SET(CPACK_PACKAGE_EXECUTABLES "lmms" "LMMS binary")
ENDIF(WIN32)

SET(MACOSX_BUNDLE_ICON_FILE "${CMAKE_SOURCE_DIR}/data/themes/default/icon.png")
SET(MACOSX_BUNDLE_GUI_IDENTIFIER "LMMS")
SET(MACOSX_BUNDLE_LONG_VERSION_STRING "${VERSION}")
SET(MACOSX_BUNDLE_BUNDLE_NAME "Linux MultiMedia Studio")
SET(MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION}")
SET(MACOSX_BUNDLE_BUNDLE_VERSION "${VERSION}")
SET(MACOSX_BUNDLE_COPYRIGHT "Tobias Doerffel, 2008")

SET(CPACK_SOURCE_GENERATOR "TBZ2")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "lmms-${VERSION}")
INCLUDE(CPack)

