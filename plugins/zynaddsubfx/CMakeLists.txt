INCLUDE(BuildPlugin)


# definitions for ZynAddSubFX
IF(LMMS_BUILD_LINUX OR LMMS_BUILD_APPLE OR LMMS_BUILD_OPENBSD)
	FIND_PACKAGE(X11)
	INCLUDE_DIRECTORIES(${X11_INCLUDE_DIR})
	ADD_DEFINITIONS(-DOS_LINUX)
ELSE()
	ADD_DEFINITIONS(-DOS_WINDOWS)
ENDIF()

# do not conflict with LMMS' Controller class
ADD_DEFINITIONS(-DController=ZynController)

# use asm optimizations when on x86 or x86_64
IF(LMMS_HOST_X86 OR LMMS_HOST_X86_64)
	ADD_DEFINITIONS(-DASM_F2I_YES)
ENDIF(LMMS_HOST_X86 OR LMMS_HOST_X86_64)

# build ZynAddSubFX with full optimizations
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wno-write-strings -Wno-deprecated-declarations -fpermissive")

# Enable C++11, but only for ZynAddSubFx.cpp
set_property(SOURCE ZynAddSubFx.cpp APPEND_STRING PROPERTY COMPILE_FLAGS " -std=c++0x ")

# link system-libraries when on win32
IF(LMMS_BUILD_WIN32)
	ADD_DEFINITIONS(-DPTW32_STATIC_LIB)
ENDIF(LMMS_BUILD_WIN32)

SET(FLTK_SKIP_OPENGL TRUE)
SET(FLTK_SKIP_FORMS TRUE)
SET(FLTK_SKIP_IMAGES TRUE)
SET(FLTK_SKIP_MATH TRUE)
IF(MINGW_PREFIX)
	SET(FLTK_SKIP_FLUID TRUE)
ENDIF()

FIND_PACKAGE(FLTK)

IF(NOT FLTK_FOUND)
	RETURN()
ENDIF()

IF(MINGW_PREFIX)
	SET(FLTK_FLUID_EXECUTABLE "${MINGW_PREFIX}/bin/fluid")
ENDIF()

IF(NOT EXISTS ${FLTK_FLUID_EXECUTABLE})
    # Try to detect full path anyway
    EXECUTE_PROCESS(COMMAND which fluid OUTPUT_VARIABLE FLTK_FLUID_EXECUTABLE OUTPUT_STRIP_TRAILING_WHITESPACE)
    IF(NOT EXISTS ${FLTK_FLUID_EXECUTABLE})
        MESSAGE(FATAL_ERROR "Please install the fluid binary which is part of FLTK.")
    ENDIF()
ENDIF()

INCLUDE_DIRECTORIES("${FLTK_INCLUDE_DIR}"
                    "${CMAKE_CURRENT_SOURCE_DIR}"
                    ${FFTW3F_INCLUDE_DIRS}
                    "${CMAKE_CURRENT_BINARY_DIR}"
                    "${CMAKE_BINARY_DIR}")

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/zynaddsubfx/src/UI)

ADD_DEFINITIONS(-DPLUGINVERSION)	# removes exit confirmation dialogs etc. in MasterUI.fl
add_subdirectory(zynaddsubfx/src/Nio)
add_subdirectory(zynaddsubfx/src/UI)

SET(zynaddsubfx_core_SRCS
    zynaddsubfx/src/DSP/AnalogFilter.cpp
    zynaddsubfx/src/DSP/FFTwrapper.cpp
    zynaddsubfx/src/DSP/Filter.cpp
    zynaddsubfx/src/DSP/FormantFilter.cpp
    zynaddsubfx/src/DSP/SVFilter.cpp
    zynaddsubfx/src/DSP/Unison.cpp
    zynaddsubfx/src/Effects/Alienwah.cpp
	zynaddsubfx/src/Effects/Chorus.cpp
	zynaddsubfx/src/Effects/Distorsion.cpp
	zynaddsubfx/src/Effects/DynamicFilter.cpp
	zynaddsubfx/src/Effects/Echo.cpp
	zynaddsubfx/src/Effects/Effect.cpp
	zynaddsubfx/src/Effects/EffectLFO.cpp
	zynaddsubfx/src/Effects/EffectMgr.cpp
	zynaddsubfx/src/Effects/EQ.cpp
	zynaddsubfx/src/Effects/Phaser.cpp
	zynaddsubfx/src/Effects/Reverb.cpp
	zynaddsubfx/src/Misc/Bank.cpp
	zynaddsubfx/src/Misc/Config.cpp
	zynaddsubfx/src/Misc/Dump.cpp
	zynaddsubfx/src/Misc/Master.cpp
	zynaddsubfx/src/Misc/Microtonal.cpp
	zynaddsubfx/src/Misc/Part.cpp
	zynaddsubfx/src/Misc/Util.cpp
	zynaddsubfx/src/Misc/QtXmlWrapper.cpp
	zynaddsubfx/src/Misc/Recorder.cpp
	zynaddsubfx/src/Misc/WavFile.cpp
	zynaddsubfx/src/Misc/WaveShapeSmps.cpp
	zynaddsubfx/src/Params/ADnoteParameters.cpp
	zynaddsubfx/src/Params/Controller.cpp
	zynaddsubfx/src/Params/EnvelopeParams.cpp
	zynaddsubfx/src/Params/FilterParams.cpp
	zynaddsubfx/src/Params/LFOParams.cpp
	zynaddsubfx/src/Params/PADnoteParameters.cpp
	zynaddsubfx/src/Params/Presets.cpp
	zynaddsubfx/src/Params/PresetsArray.cpp
	zynaddsubfx/src/Params/PresetsStore.cpp
	zynaddsubfx/src/Params/SUBnoteParameters.cpp
	zynaddsubfx/src/Synth/SynthNote.cpp
	zynaddsubfx/src/Synth/ADnote.cpp
	zynaddsubfx/src/Synth/Envelope.cpp
	zynaddsubfx/src/Synth/LFO.cpp
	zynaddsubfx/src/Synth/OscilGen.cpp
	zynaddsubfx/src/Synth/PADnote.cpp
	zynaddsubfx/src/Synth/Resonance.cpp
	zynaddsubfx/src/Synth/SUBnote.cpp
    )


IF(LMMS_BUILD_LINUX)
	ADD_LIBRARY(ZynAddSubFxCore MODULE LocalZynAddSubFx.cpp ${zynaddsubfx_core_SRCS})
ELSE()
	ADD_LIBRARY(ZynAddSubFxCore SHARED LocalZynAddSubFx.cpp ${zynaddsubfx_core_SRCS})
ENDIF()
TARGET_LINK_LIBRARIES(ZynAddSubFxCore zynaddsubfx_nio ${FFTW3F_LIBRARIES} ${QT_LIBRARIES} -lz -lpthread)

# required libs for debug msys builds
IF(LMMS_BUILD_MSYS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
	TARGET_LINK_LIBRARIES(ZynAddSubFxCore QtCore4 QtGui4 QtXml4)
ENDIF()

IF(QT5)
TARGET_LINK_LIBRARIES(ZynAddSubFxCore Qt5::Widgets Qt5::Xml)
ENDIF()

IF(LMMS_BUILD_WIN32)
	TARGET_LINK_LIBRARIES(ZynAddSubFxCore -lws2_32)
	INSTALL(TARGETS ZynAddSubFxCore RUNTIME DESTINATION "${PLUGIN_DIR}")
ELSE(LMMS_BUILD_WIN32)
	INSTALL(TARGETS ZynAddSubFxCore LIBRARY DESTINATION "${PLUGIN_DIR}")
ENDIF(LMMS_BUILD_WIN32)

IF(LMMS_BUILD_LINUX)
	LINK_LIBRARIES(-Wl,--enable-new-dtags)
	SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
	SET(CMAKE_INSTALL_RPATH "$ORIGIN")
ELSE()
	SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${PLUGIN_DIR}")
ENDIF()
BUILD_PLUGIN(zynaddsubfx ZynAddSubFx.cpp ZynAddSubFx.h MOCFILES ZynAddSubFx.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
TARGET_LINK_LIBRARIES(zynaddsubfx -L.. -lZynAddSubFxCore)
ADD_DEPENDENCIES(zynaddsubfx ZynAddSubFxCore)

IF(WIN32)
	SET(WINRC "${CMAKE_CURRENT_BINARY_DIR}/zynaddsubfxrc.obj")
	ADD_CUSTOM_COMMAND(OUTPUT "${WINRC}"
				COMMAND "${WINDRES}"
					"-I\"${CMAKE_CURRENT_SOURCE_DIR}\""
					"-o\"${CMAKE_CURRENT_BINARY_DIR}/zynaddsubfxrc.obj\""
					"-i\"${CMAKE_CURRENT_BINARY_DIR}/zynaddsubfx.rc\""
				DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/zynaddsubfx.rc")
ENDIF(WIN32)

IF(NOT LMMS_BUILD_LINUX)
	SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${PLUGIN_DIR}")
	SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
ENDIF()
ADD_EXECUTABLE(RemoteZynAddSubFx RemoteZynAddSubFx.cpp "${WINRC}")
INSTALL(TARGETS RemoteZynAddSubFx RUNTIME DESTINATION "${PLUGIN_DIR}")

# Remove useless dependencies from FLTK. Use fltk-config to avoid static library
# in older environments
SET(FLTK_FILTERED_LDFLAGS ${FLTK_LIBRARIES})
FIND_PROGRAM(FLTK_CONFIG fltk-config)
IF(FLTK_CONFIG AND NOT (LMMS_BUILD_APPLE OR LMMS_BUILD_WIN32))
	EXECUTE_PROCESS(COMMAND ${FLTK_CONFIG} --ldflags OUTPUT_VARIABLE FLTK_FILTERED_LDFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
	STRING(REPLACE " " ";" FLTK_FILTERED_LDFLAGS ${FLTK_FILTERED_LDFLAGS})
	LIST(REMOVE_ITEM FLTK_FILTERED_LDFLAGS -lX11)
ENDIF()
TARGET_LINK_LIBRARIES(RemoteZynAddSubFx zynaddsubfx_gui -L.. -lZynAddSubFxCore ${FLTK_FILTERED_LDFLAGS} -lpthread )
ADD_DEPENDENCIES(RemoteZynAddSubFx ZynAddSubFxCore)

# Support qt_version_tag in Qt 5.6
IF(QT5)
	TARGET_LINK_LIBRARIES(RemoteZynAddSubFx Qt5::Core)
ENDIF(QT5)

# link Qt libraries when on win32
IF(LMMS_BUILD_WIN32)
	TARGET_LINK_LIBRARIES(RemoteZynAddSubFx ${QT_LIBRARIES})
ENDIF(LMMS_BUILD_WIN32)


IF(LMMS_BUILD_WIN32)
	ADD_CUSTOM_COMMAND(TARGET ZynAddSubFxCore POST_BUILD COMMAND "${STRIP}" \"$<TARGET_FILE:ZynAddSubFxCore>\")
	ADD_CUSTOM_COMMAND(TARGET RemoteZynAddSubFx POST_BUILD COMMAND "${STRIP}" \"$<TARGET_FILE:RemoteZynAddSubFx>\")
ENDIF(LMMS_BUILD_WIN32)
