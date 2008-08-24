CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

PROJECT(lmms)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
SET(CMAKE_BUILD_TYPE relwithdebinfo)

IF(COMMAND CMAKE_POLICY)
	CMAKE_POLICY(SET CMP0005 NEW)
	CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

INCLUDE(AddFileDependencies)
INCLUDE(CheckIncludeFiles)
INCLUDE(FindPkgConfig)

SET(VERSION_MAJOR "0")
SET(VERSION_MINOR "4")
SET(VERSION_PATCH "0")
SET(VERSION_SUFFIX "beta2")
SET(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
IF(VERSION_SUFFIX)
	SET (VERSION "${VERSION}-${VERSION_SUFFIX}")
ENDIF(VERSION_SUFFIX)
ADD_DEFINITIONS(-D'LMMS_VERSION="${VERSION}"')
IF(WIN32)
	SET(LMMS_BUILD_WIN32 1)
ELSE(WIN32)
	IF(APPLE)
		SET(LMMS_BUILD_APPLE 1)
	ELSE(APPLE)
		SET(LMMS_BUILD_LINUX 1)
	ENDIF(APPLE)
ENDIF(WIN32)


OPTION(WANT_ALSA	"Include ALSA (Advanced Linux Sound Architecture) support" ON)
OPTION(WANT_CAPS	"Include C* Audio Plugin Suite (LADSPA plugins)" ON)
OPTION(WANT_FFTW3F	"Include SpectrumAnalyzer plugin" ON)
OPTION(WANT_JACK	"Include JACK (Jack Audio Connection Kit) support" ON)
OPTION(WANT_OGGVORBIS	"Include OGG/Vorbis support" ON)
OPTION(WANT_PULSEAUDIO	"Include PulseAudio support" ON)
OPTION(WANT_SDL		"Include SDL (Simple DirectMedia Layer) support" ON)
OPTION(WANT_SF2		"Include SoundFont2 player plugin" ON)
OPTION(WANT_STK		"Include Stk (Synthesis Toolkit) support" ON)
OPTION(WANT_SYSTEM_SR	"Use system's libsamplerate" ON)
OPTION(WANT_TAP		"Include Tom's Audio Processing (LADSPA plugins)" ON)
OPTION(WANT_VST		"Include VST support" ON)
OPTION(WANT_WINMM	"Include WinMM MIDI support" OFF)
OPTION(WANT_PCH		"Use precompiled headers" OFF)

IF(LMMS_BUILD_WIN32)
	SET(WANT_ALSA OFF)
	SET(WANT_JACK OFF)
	SET(WANT_PULSEAUDIO OFF)
	SET(WANT_STK OFF)
	SET(WANT_SYSTEM_SR OFF)
	SET(WANT_WINMM ON)
	SET(LMMS_HAVE_WINMM TRUE)
	SET(STATUS_ALSA "<not supported on this platform>")
	SET(STATUS_JACK "<not supported on this platform>")
	SET(STATUS_PULSEAUDIO "<not supported on this platform>")
	SET(STATUS_STK "<not supported on this platform>")
	SET(STATUS_WINMM "OK")
ELSE(LMMS_BUILD_WIN32)
	SET(STATUS_WINMM "<not supported on this platform>")
ENDIF(LMMS_BUILD_WIN32)

MESSAGE("PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
EXEC_PROGRAM( uname ARGS "-m"OUTPUT_VARIABLE Machine )
MESSAGE("Machine: ${Machine}")
IF(${Machine} MATCHES "i686" OR ${Machine} MATCHES "i386" )
	MESSAGE("-- This is an 32 bit machine")
	SET(HOST_IS_64BIT FALSE)
ELSE(${Machine} MATCHES "i686" OR ${Machine} MATCHES "i386" )
	IF(${Machine} MATCHES "x86_64")
		MESSAGE("-- This is an 64 bit machine")
		SET(HOST_IS_64BIT TRUE)
	ELSE(${Machine} MATCHES "x86_64")
		MESSAGE("Can't identify this machine. Assuming 32 bit platform ")
		SET(HOST_IS_64BIT FALSE)
	ENDIF(${Machine} MATCHES "x86_64")
ENDIF(${Machine} MATCHES "i686" OR ${Machine} MATCHES "i386" )


IF(HOST_IS_64BIT)
	SET(LIB_DIR lib64)
ELSE(HOST_IS_64BIT)
	SET(LIB_DIR lib)
ENDIF(HOST_IS_64BIT)

IF(LMMS_BUILD_WIN32)
	SET(PLUGIN_DIR plugins)
	SET(DATA_DIR data)
	SET(LMMS_DATA_DIR data)
ELSE(LMMS_BUILD_WIN32)
	SET(PLUGIN_DIR ${LIB_DIR}/lmms)
	SET(DATA_DIR share)
	SET(LMMS_DATA_DIR ${DATA_DIR}/lmms)
ENDIF(LMMS_BUILD_WIN32)


CHECK_INCLUDE_FILES(stdint.h LMMS_HAVE_STDINT_H)
CHECK_INCLUDE_FILES(stdlib.h LMMS_HAVE_STDLIB_H)
CHECK_INCLUDE_FILES(pthread.h LMMS_HAVE_PTHREAD_H)
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


# check for Qt4
FIND_PACKAGE(Qt4 REQUIRED)
SET(QT_USE_QTXML 1)
STRING(REGEX REPLACE "[^/]+$" "translations/" QT_TRANSLATIONS_DIR "${QT_MKSPECS_DIR}")
IF(EXISTS "${QT_TRANSLATIONS_DIR}")
	ADD_DEFINITIONS(-D'QT_TRANSLATIONS_DIR="${QT_TRANSLATIONS_DIR}"')
ENDIF(EXISTS "${QT_TRANSLATIONS_DIR}")
IF(NOT WIN32)
	STRING(REPLACE "-DQT_DLL" "" QT_DEFINITIONS "${QT_DEFINITIONS}")
ENDIF(NOT WIN32)
INCLUDE(${QT_USE_FILE})

# check for libsndfile
PKG_CHECK_MODULES(SNDFILE REQUIRED sndfile>=1.0.11)
IF(NOT SNDFILE_FOUND)
	MESSAGE(FATAL_ERROR "LMMS requires libsndfile1 and libsndfile1-dev >= 1.0.11 - please install and try again!")
ENDIF(NOT SNDFILE_FOUND)

IF(WANT_CAPS)
SET(LMMS_HAVE_CAPS TRUE)
SET(STATUS_CAPS "OK")
ELSE(WANT_CAPS)
SET(STATUS_CAPS "not built as requested")
ENDIF(WANT_CAPS)

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
		SET(STATUS_SDL "not found, please install libsdl1.2-dev (or similiar)")
	ENDIF(SDL_FOUND)
ENDIF(WANT_SDL)
IF(NOT LMMS_HAVE_SDL)
	SET(SDL_INCLUDE_DIR "")
ELSE(NOT LMMS_HAVE_SDL)
	IF(NOT SDL_INCLUDE_DIR)
		SET(SDL_INCLUDE_DIR ${CMAKE_FIND_ROOT_PATH}/include)
	ENDIF(NOT SDL_INCLUDE_DIR)
ENDIF(NOT LMMS_HAVE_SDL)


# check for Stk
IF(WANT_STK)
	FIND_PACKAGE(STK)
	IF(STK_FOUND)
		SET(LMMS_HAVE_STK TRUE)
		SET(STATUS_STK "OK")
	ELSE(STK_FOUND)
		SET(STK_INCLUDE_DIR "")
		SET(STATUS_STK "not found, please install libstk0-dev (or similiar)")
	ENDIF(STK_FOUND)
ENDIF(WANT_STK)


# check for PulseAudio
IF(WANT_PULSEAUDIO)
	FIND_PACKAGE(PulseAudio)
	IF(PULSEAUDIO_FOUND)
		SET(LMMS_HAVE_PULSEAUDIO TRUE)
		SET(STATUS_PULSEAUDIO "OK")
	ELSE(PULSEAUDIO_FOUND)
		SET(STATUS_PULSEAUDIO "not found, please install libpulse-dev (or similiar)")
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
		SET(STATUS_OGGVORBIS "not found, please install libogg-dev and libvorbis-dev (or similiar)")
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
	INCLUDE(FindAlsa)
	ALSA_CONFIGURE_FILE(${CMAKE_BINARY_DIR}/alsaconfig.h)
	IF(ALSA_FOUND)
		SET(LMMS_HAVE_ALSA TRUE)
		SET(STATUS_ALSA "OK")
	ELSE(ALSA_FOUND)
		SET(STATUS_ALSA "not found, please install libasound2-dev (or similiar)")
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
		SET(STATUS_JACK "not found, please install libjack0.100.0-dev (or similiar)")
	ENDIF(JACK_FOUND)
ENDIF(WANT_JACK)


# check for FFTW3F-library
IF(WANT_FFTW3F)
	PKG_CHECK_MODULES(FFTW3F fftw3f>=3.0.0)
	IF(FFTW3F_FOUND)
		SET(LMMS_HAVE_FFTW3F TRUE)
		SET(STATUS_FFTW3F "OK")
	ELSE(FFTW3F_FOUND)
		SET(STATUS_FFTW3F "not found, please install libfftw3-dev (or similiar)")
	ENDIF(FFTW3F_FOUND)
ENDIF(WANT_FFTW3F)


# check for Fluidsynth
IF(WANT_SF2)
	PKG_CHECK_MODULES(FLUIDSYNTH fluidsynth>=1.0.7)
	IF(FLUIDSYNTH_FOUND)
		SET(LMMS_HAVE_FLUIDSYNTH TRUE)
		SET(STATUS_FLUIDSYNTH "OK")
	ELSE(FLUIDSYNTH_FOUND)
		SET(STATUS_FLUIDSYNTH "not found, please install libfluidsynth-dev (or similiar)")
	ENDIF(FLUIDSYNTH_FOUND)
ENDIF(WANT_SF2)


# check for WINE
if(WANT_VST)
	INCLUDE(CheckLibraryExists)
	INCLUDE(CheckIncludeFileCXX)
	SET(CMAKE_REQUIRED_FLAGS_ORIG ${CMAKE_REQUIRED_FLAGS})
	SET(CMAKE_REQUIRED_INCLUDES_ORIG ${CMAKE_REQUIRED_INCLUDES})
	SET(CMAKE_CXX_COMPILER_ORIG ${CMAKE_CXX_COMPILER})
	IF(HOST_IS_64BIT)
		SET(CMAKE_REQUIRED_FLAGS -m32 ${CMAKE_REQUIRED_FLAGS})
	ENDIF(HOST_IS_64BIT)
	CHECK_LIBRARY_EXISTS(wine wine_init "" HAVE_LIBWINE)
	SET(CMAKE_CXX_COMPILER /usr/bin/wineg++)
	SET(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${CMAKE_INSTALL_PREFIX}/include/wine/windows /usr/include/wine/windows)
	CHECK_INCLUDE_FILE_CXX(windows.h HAVE_WINDOWS_H)
	SET(CMAKE_CXX_COMPILER ${CMAKE_CXX_COMPILER_ORIG})
	SET(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS_ORIG})
	SET(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES_ORIG})
	IF(HAVE_LIBWINE AND HAVE_WINDOWS_H)
		SET(LMMS_HAVE_VST TRUE)
		SET(STATUS_WINE "OK")
	ELSE(HAVE_LIBWINE AND HAVE_WINDOWS_H)
		SET(STATUS_WINE "not found, please install (lib)wine-dev (or similiar) and gcc-multilib (only for 64 bit systems)")
	ENDIF(HAVE_LIBWINE AND HAVE_WINDOWS_H)
ENDIF(WANT_VST)


# check for libsamplerate
IF(WANT_SYSTEM_SR)
	PKG_CHECK_MODULES(SAMPLERATE samplerate>=0.1.3)
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
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/lmms.rc.in ${CMAKE_BINARY_DIR}/lmms.rc)

# people simply updating SVN will still have this and mess up build with it
FILE(REMOVE include/lmmsconfig.h)

FILE(GLOB lmms_INCLUDES include/*.h)
FILE(GLOB lmms_UI src/gui/dialogs/*.ui)
FILE(GLOB_RECURSE lmms_SOURCES src/*.cpp)

SET(lmms_MOC ${lmms_INCLUDES})

SET(lmms_EMBEDDED_RESOURCES ${CMAKE_SOURCE_DIR}/AUTHORS ${CMAKE_SOURCE_DIR}/COPYING)

QT4_WRAP_CPP(lmms_MOC_out ${lmms_MOC})
QT4_WRAP_UI(lmms_UI_out ${lmms_UI})


# embedded resources stuff
ADD_EXECUTABLE(bin2res buildtools/bin2res.cpp)
GET_TARGET_PROPERTY(BIN2RES bin2res LOCATION)
SET(LMMS_ER_H ${CMAKE_CURRENT_BINARY_DIR}/embedded_resources.h)

# we somehow have to make LMMS-binary depend on MOC-files
ADD_FILE_DEPENDENCIES(lmmsconfig.h ${lmms_MOC_out})

ADD_CUSTOM_COMMAND(OUTPUT ${LMMS_ER_H} COMMAND ${BIN2RES} ARGS ${lmms_EMBEDDED_RESOURCES} > ${LMMS_ER_H} DEPENDS ${BIN2RES})

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions ${CXXFLAGS}")
IF(WIN32)
	SET(WINRC "${CMAKE_BINARY_DIR}/lmmsrc.obj")
	ADD_CUSTOM_COMMAND(OUTPUT ${WINRC}
				COMMAND /opt/mingw/bin/i586-mingw32-windres
					-I${CMAKE_SOURCE_DIR}
					-o${CMAKE_BINARY_DIR}/lmmsrc.obj
					-i${CMAKE_BINARY_DIR}/lmms.rc
				DEPENDS ${CMAKE_BINARY_DIR}/lmms.rc)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes")
ELSE(WIN32)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
ENDIF(WIN32)

# make sub-directories
ADD_SUBDIRECTORY(plugins)
ADD_SUBDIRECTORY(data)

#
# build LMMS-binary
#
ADD_DEFINITIONS(-D'LIB_DIR="${CMAKE_INSTALL_PREFIX}/${LIB_DIR}/"' -D'PLUGIN_DIR="${CMAKE_INSTALL_PREFIX}/${LIB_DIR}/lmms/"' ${PULSEAUDIO_DEFINITIONS})

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR}/include ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include ${SDL_INCLUDE_DIR} ${PULSEAUDIO_INCLUDE_DIR} ${JACK_INCLUDE_DIRS} ${OGGVORBIS_INCLUDE_DIR} ${SAMPLERATE_INCLUDE_DIRS} ${SNDFILE_INCLUDE_DIRS})
LINK_DIRECTORIES(${CMAKE_INSTALL_PREFIX}/lib ${ASOUND_LIBRARY_DIR} ${JACK_LIBRARY_DIRS} ${SAMPLERATE_LIBRARY_DIRS} ${SNDFILE_LIBRARY_DIRS})
LINK_LIBRARIES(${QT_LIBRARIES} ${ASOUND_LIBRARY} ${SDL_LIBRARY} ${PULSEAUDIO_LIBRARIES} ${JACK_LIBRARIES} ${OGGVORBIS_LIBRARIES} ${SAMPLERATE_LIBRARIES} ${SNDFILE_LIBRARIES})

ADD_EXECUTABLE(lmms ${lmms_SOURCES} ${lmms_INCLUDES} ${LIBSAMPLERATE_SOURCES} ${LMMS_ER_H} ${lmms_UI_out} lmmsconfig.h ${WINRC})
IF(LMMS_BUILD_WIN32)
	SET_TARGET_PROPERTIES(lmms PROPERTIES LINK_FLAGS "${LINK_FLAGS} -mwindows")
	ADD_CUSTOM_COMMAND(TARGET lmms POST_BUILD COMMAND /opt/mingw/bin/i586-mingw32-strip ${CMAKE_BINARY_DIR}/lmms.exe)
	INSTALL(TARGETS lmms RUNTIME DESTINATION .)
	INSTALL(FILES /opt/mingw/bin/QtCore4.dll /opt/mingw/bin/QtGui4.dll /opt/mingw/bin/QtXml4.dll /opt/mingw/bin/libz.dll /opt/mingw/bin/libsndfile-1.dll /opt/mingw/bin/libvorbis-0.dll /opt/mingw/bin/libvorbisenc-2.dll /opt/mingw/bin/libvorbisfile-3.dll /opt/mingw/bin/libogg-0.dll /opt/mingw/bin/libfluidsynth-1.dll /opt/mingw/bin/libfftw3f-3.dll /opt/mingw/bin/SDL.dll /opt/mingw/i586-mingw32/bin/mingwm10.dll DESTINATION .)
ELSE(LMMS_BUILD_WIN32)
	INSTALL(TARGETS lmms RUNTIME DESTINATION bin)
ENDIF(LMMS_BUILD_WIN32)

#
# rules for building localizations
#
FILE(GLOB lmms_LOCALES data/locale/*.ts)
SET(locale_targets "")
FOREACH(_ts_file ${lmms_LOCALES})
	STRING(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/data/locale/" "" _ts_target "${_ts_file}")
	STRING(REPLACE ".ts" ".qm" _qm_file "${_ts_file}")
	STRING(REPLACE ".ts" ".qm" _qm_target "${_ts_target}")
	ADD_CUSTOM_TARGET(${_ts_target} COMMAND ${QT_LUPDATE_EXECUTABLE} ${lmms_SOURCES} `find plugins/ -type f -name '*.cpp'` -ts ${_ts_file})
	ADD_CUSTOM_TARGET(${_qm_target} COMMAND ${QT_LRELEASE_EXECUTABLE} ${_ts_file} -qm ${_qm_file})
	LIST(APPEND locale_targets "${_ts_target}")
ENDFOREACH(_ts_file ${lmms_LOCALES})
ADD_CUSTOM_TARGET(update-locales)# DEPENDS ${locale_targets}) # does not work
FOREACH(_item ${locale_targets})
	ADD_DEPENDENCIES(update-locales ${_item})
ENDFOREACH(_item ${locale_targets})



#
# add distclean-target
#
ADD_CUSTOM_TARGET(distclean
			COMMAND make clean
			COMMAND rm -rf `find -name cmake_install.cmake` `find -name Makefile` `find -type d -name CMakeFiles` CMakeCache.txt lmmsconfig.h alsaconfig.h)

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
			COMMAND mv tmp/lmms/opt/mingw/share/lmms/* tmp/lmms/data/ && rm -rf tmp/lmms/opt
			COMMAND cp /opt/mingw/bin/QtCore4.dll tmp/lmms
			COMMAND cp /opt/mingw/bin/QtGui4.dll tmp/lmms
			COMMAND cp /opt/mingw/bin/QtXml4.dll tmp/lmms
			COMMAND cp /opt/mingw/bin/libz.dll tmp/lmms
			COMMAND cp /opt/mingw/bin/libsndfile-1.dll tmp/lmms
			COMMAND cp /opt/mingw/bin/libvorbis*.dll tmp/lmms
			COMMAND cp /opt/mingw/bin/libogg-0.dll tmp/lmms
			COMMAND cp /opt/mingw/bin/libfluidsynth-1.dll tmp/lmms
			COMMAND cp /opt/mingw/bin/libfftw3f-3.dll tmp/lmms
			COMMAND cp /opt/mingw/bin/SDL.dll tmp/lmms
			COMMAND cp /opt/mingw/i586-mingw32/bin/mingwm10.dll tmp/lmms
			COMMAND cp -L ${CMAKE_SOURCE_DIR}/COPYING tmp/lmms/LICENSE.TXT
			COMMAND cp -L ${CMAKE_SOURCE_DIR}/README tmp/lmms/README.TXT
			COMMAND /opt/mingw/bin/i586-mingw32-strip tmp/lmms/lmms.exe tmp/lmms/plugins/*.dll tmp/lmms/plugins/ladspa/*.dll
			COMMAND mv tmp/lmms tmp/lmms-${VERSION}
			COMMAND cd tmp && zip -r -9 ../lmms-${VERSION}-bin-win32.zip lmms-${VERSION}/*
			COMMAND rm -rf tmp
)


IF(WANT_PCH)
    FIND_PACKAGE(PCHSupport)
    IF(PCHSupport_FOUND)
	SET(USE_PCH TRUE)
        ADD_DEFINITIONS(-DUSE_PCH)
        MESSAGE(STATUS "Enabling precompiled headers for GCC ${gcc_compiler_version}")
	ADD_PRECOMPILED_HEADER(lmms pch include/pch.h)
    ENDIF(PCHSupport_FOUND)
ENDIF(WANT_PCH)

SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${LMMS_ER_H} ${lmms_MOC_out} ${lmms_UI_out} lmmsconfig.h alsaconfig.h pch.h.gch/")


#
# display configuration information
#

MESSAGE("\n"
"Installation Summary\n"
"--------------------\n"
"* Install Directory           : ${CMAKE_INSTALL_PREFIX}\n"
"* Build type                  : ${CMAKE_BUILD_TYPE}\n"
"* Use precompiled headers     : ${USE_PCH}\n"
"* Use system's libsamplerate  : ${LMMS_HAVE_SAMPLERATE}\n"
)

MESSAGE(
"Supported audio interfaces\n"
"--------------------------\n"
"* ALSA                        : ${STATUS_ALSA}\n"
"* JACK                        : ${STATUS_JACK}\n"
"* OSS                         : ${STATUS_OSS}\n"
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
)

MESSAGE(
"Optional plugins\n"
"----------------\n"
"* SoundFont2 player           : ${STATUS_FLUIDSYNTH}\n"
"* Stk Mallets                 : ${STATUS_STK}\n"
"* VST-instrument hoster       : ${STATUS_WINE}\n"
"* VST-effect hoster           : ${STATUS_WINE}\n"
"* SpectrumAnalyzer            : ${STATUS_FFTW3F}\n"
"* CAPS LADSPA plugins         : ${STATUS_CAPS}\n"
"* TAP LADSPA plugins          : ${STATUS_TAP}\n"
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
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
SET(CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}-${VERSION_SUFFIX}")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "LMMS-${VERSION}")
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

