INCLUDE(BuildPlugin)


# definitions for ZynAddSubFX
IF(LMMS_BUILD_LINUX OR LMMS_BUILD_APPLE)
	ADD_DEFINITIONS(-DOS_LINUX)
ELSE(LMMS_BUILD_LINUX OR LMMS_BUILD_APPLE)
	ADD_DEFINITIONS(-DOS_WINDOWS)
ENDIF(LMMS_BUILD_LINUX OR LMMS_BUILD_APPLE)

# do not conflict with LMMS' Controller class
ADD_DEFINITIONS(-DController=ZynController)

# use asm optimizations when on x86 or x86_64
IF(LMMS_HOST_X86 OR LMMS_HOST_X86_64)
	ADD_DEFINITIONS(-DASM_F2I_YES)
ENDIF(LMMS_HOST_X86 OR LMMS_HOST_X86_64)

# build ZynAddSubFX with full optimizations
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wno-write-strings -Wno-deprecated-declarations")

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

FIND_PACKAGE(FLTK REQUIRED)

IF(MINGW_PREFIX)
	SET(FLTK_FLUID_EXECUTABLE "${MINGW_PREFIX}/bin/fluid")
ENDIF()

IF(NOT EXISTS ${FLTK_FLUID_EXECUTABLE})
	MESSAGE(FATAL_ERROR "Please install the fluid binary which is part of FLTK.")
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


ADD_LIBRARY(ZynAddSubFxCore SHARED LocalZynAddSubFx.cpp ${zynaddsubfx_core_SRCS})
TARGET_LINK_LIBRARIES(ZynAddSubFxCore zynaddsubfx_nio ${FFTW3F_LIBRARIES} ${QT_LIBRARIES} -lz -lpthread)
IF(LMMS_BUILD_WIN32)
	TARGET_LINK_LIBRARIES(ZynAddSubFxCore -lws2_32)
	INSTALL(TARGETS ZynAddSubFxCore RUNTIME DESTINATION "${PLUGIN_DIR}")
ELSE(LMMS_BUILD_WIN32)
	INSTALL(TARGETS ZynAddSubFxCore LIBRARY DESTINATION "${PLUGIN_DIR}")
ENDIF(LMMS_BUILD_WIN32)

BUILD_PLUGIN(zynaddsubfx ZynAddSubFx.cpp ZynAddSubFx.h MOCFILES ZynAddSubFx.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
TARGET_LINK_LIBRARIES(zynaddsubfx ZynAddSubFxCore)

IF(WIN32)
	SET(WINRC "${CMAKE_CURRENT_BINARY_DIR}/zynaddsubfxrc.obj")
	ADD_CUSTOM_COMMAND(OUTPUT "${WINRC}"
				COMMAND "${WINDRES}"
					"-I\"${CMAKE_CURRENT_SOURCE_DIR}\""
					"-o\"${CMAKE_CURRENT_BINARY_DIR}/zynaddsubfxrc.obj\""
					"-i\"${CMAKE_CURRENT_BINARY_DIR}/zynaddsubfx.rc\""
				DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/zynaddsubfx.rc")
ENDIF(WIN32)

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${PLUGIN_DIR}")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
ADD_EXECUTABLE(RemoteZynAddSubFx RemoteZynAddSubFx.cpp "${WINRC}")
INSTALL(TARGETS RemoteZynAddSubFx RUNTIME DESTINATION "${PLUGIN_DIR}")
TARGET_LINK_LIBRARIES(RemoteZynAddSubFx zynaddsubfx_gui ZynAddSubFxCore ${FLTK_LIBRARIES} -lpthread )

# link Qt libraries when on win32
IF(LMMS_BUILD_WIN32)
	TARGET_LINK_LIBRARIES(RemoteZynAddSubFx ${QT_LIBRARIES})
ENDIF(LMMS_BUILD_WIN32)

# FLTK needs X
IF(LMMS_BUILD_LINUX)
	TARGET_LINK_LIBRARIES(RemoteZynAddSubFx -ldl)
ENDIF(LMMS_BUILD_LINUX)


IF(LMMS_BUILD_WIN32)
	ADD_CUSTOM_COMMAND(TARGET ZynAddSubFxCore POST_BUILD COMMAND "${STRIP}" "\"${CMAKE_CURRENT_BINARY_DIR}/libZynAddSubFxCore.dll\"")
	ADD_CUSTOM_COMMAND(TARGET RemoteZynAddSubFx POST_BUILD COMMAND "${STRIP}" "\"${CMAKE_CURRENT_BINARY_DIR}/RemoteZynAddSubFx.exe\"")
ENDIF(LMMS_BUILD_WIN32)
