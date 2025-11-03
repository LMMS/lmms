if(NOT FLTK_FOUND)
	return()
endif()

INCLUDE(BuildPlugin)

find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# definitions for ZynAddSubFX
IF(LMMS_BUILD_LINUX OR LMMS_BUILD_APPLE OR LMMS_BUILD_OPENBSD OR LMMS_BUILD_FREEBSD)
	FIND_PACKAGE(X11)
	include_directories(SYSTEM ${X11_INCLUDE_DIR})
	ADD_DEFINITIONS(-DOS_LINUX)
ELSE()
	ADD_DEFINITIONS(-DOS_WINDOWS)
ENDIF()

# use asm optimizations when on x86 or x86_64
if(NOT MSVC AND (LMMS_HOST_X86 OR LMMS_HOST_X86_64))
	ADD_DEFINITIONS(-DASM_F2I_YES)
endif()

if(MSVC)
	# enable updated C++ language standards, disable certain permissive behavior
	set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /Zc:__cplusplus /permissive-")
	# workaround _WIN32 vs WIN32 differences
	add_compile_definitions(WIN32)
else()
	# build ZynAddSubFX with full optimizations, treat nonconformant code as warnings
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -fpermissive")
endif()

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

include_directories(SYSTEM
	"${FLTK_INCLUDE_DIR}"
	${FFTW3F_INCLUDE_DIRS}
)
include_directories(
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_BINARY_DIR}"
	"${CMAKE_BINARY_DIR}"
)

ADD_DEFINITIONS(-DPLUGINVERSION)	# removes exit confirmation dialogs etc. in MasterUI.fl
add_subdirectory(zynaddsubfx/src/Nio)
add_subdirectory(zynaddsubfx/src/UI)

add_library(zynaddsubfx_synth OBJECT
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

set_target_properties(zynaddsubfx_nio PROPERTIES SYSTEM TRUE)
set_target_properties(zynaddsubfx_gui PROPERTIES SYSTEM TRUE)
set_target_properties(zynaddsubfx_synth PROPERTIES SYSTEM TRUE)

# Relative include paths don't work automatically for the GUI, because the
# generated C++ files aren't in the source directory. Thus, add the expected
# source directory as an additional include directory.
target_include_directories(zynaddsubfx_gui PUBLIC zynaddsubfx/src/UI)

add_library(ZynAddSubFxCore STATIC
	LocalZynAddSubFx.cpp
)
target_include_directories(ZynAddSubFxCore SYSTEM PUBLIC
	zynaddsubfx/src
	"${CMAKE_CURRENT_BINARY_DIR}/zynaddsubfx/src"
)
target_static_libraries(ZynAddSubFxCore PUBLIC
	zynaddsubfx_nio
	zynaddsubfx_synth
)
target_link_libraries(ZynAddSubFxCore INTERFACE
	${FFTW3F_LIBRARIES}
	${QT_LIBRARIES}
	Qt${QT_VERSION_MAJOR}::Widgets
	Qt${QT_VERSION_MAJOR}::Xml
	Threads::Threads
	ZLIB::ZLIB
)

LINK_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}/..")
IF(LMMS_BUILD_LINUX)
	LINK_LIBRARIES(-Wl,--enable-new-dtags)
	SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
	SET(CMAKE_INSTALL_RPATH "$ORIGIN")
ELSE()
	SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${PLUGIN_DIR}")
ENDIF()
BUILD_PLUGIN(zynaddsubfx ZynAddSubFx.cpp ZynAddSubFx.h MOCFILES ZynAddSubFx.h EMBEDDED_RESOURCES artwork.png logo.png)
target_static_libraries(zynaddsubfx ZynAddSubFxCore)

if(MSVC)
	set(WINRC "${CMAKE_CURRENT_BINARY_DIR}/zynaddsubfx.rc")
elseif(WIN32)
	SET(WINRC "${CMAKE_CURRENT_BINARY_DIR}/zynaddsubfxrc.obj")
	ADD_CUSTOM_COMMAND(OUTPUT "${WINRC}"
				COMMAND "${CMAKE_RC_COMPILER}"
					"-I\"${CMAKE_CURRENT_SOURCE_DIR}\""
					"-o\"${CMAKE_CURRENT_BINARY_DIR}/zynaddsubfxrc.obj\""
					"-i\"${CMAKE_CURRENT_BINARY_DIR}/zynaddsubfx.rc\""
				DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/zynaddsubfx.rc")
endif()

# Use libraries in non-standard directories (e.g., another version of Qt)
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

ADD_SUBDIRECTORY("${LMMS_SOURCE_DIR}/src/common" common)

add_executable(RemoteZynAddSubFx
	RemoteZynAddSubFx.cpp
	${LMMS_COMMON_SRCS}
	"${WINRC}"
)
target_static_libraries(RemoteZynAddSubFx ZynAddSubFxCore zynaddsubfx_gui)
INSTALL(TARGETS RemoteZynAddSubFx RUNTIME DESTINATION "${PLUGIN_DIR}")
# Needed to deploy dependencies of RemoteZynAddSubFx
SET_PROPERTY(GLOBAL APPEND PROPERTY PLUGINS_BUILT "RemoteZynAddSubFx")

TARGET_COMPILE_DEFINITIONS(RemoteZynAddSubFx PRIVATE BUILD_REMOTE_PLUGIN_CLIENT)

IF(LMMS_BUILD_WIN32)
	target_link_options(RemoteZynAddSubFx PRIVATE "-mwindows")
ENDIF()

# Remove useless dependencies from FLTK. Use fltk-config to avoid static library
# in older environments
SET(FLTK_FILTERED_LDFLAGS ${FLTK_LIBRARIES})
FIND_PROGRAM(FLTK_CONFIG fltk-config)
IF(FLTK_CONFIG AND NOT (LMMS_BUILD_APPLE OR LMMS_BUILD_WIN32))
	EXECUTE_PROCESS(COMMAND ${FLTK_CONFIG} --ldflags OUTPUT_VARIABLE FLTK_FILTERED_LDFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
	STRING(REPLACE " " ";" FLTK_FILTERED_LDFLAGS ${FLTK_FILTERED_LDFLAGS})
	LIST(REMOVE_ITEM FLTK_FILTERED_LDFLAGS -lX11)
ENDIF()
target_link_libraries(RemoteZynAddSubFx ${FLTK_FILTERED_LDFLAGS})

if(LMMS_HAVE_LIBRT)
	target_link_libraries(RemoteZynAddSubFx rt)
endif()

# Support qt_version_tag in Qt 5.6
TARGET_LINK_LIBRARIES(RemoteZynAddSubFx Qt${QT_VERSION_MAJOR}::Core)

# link Qt libraries when on win32
IF(LMMS_BUILD_WIN32)
	TARGET_LINK_LIBRARIES(RemoteZynAddSubFx ${QT_LIBRARIES})
ENDIF(LMMS_BUILD_WIN32)


IF(LMMS_BUILD_WIN32)
	add_custom_command(
		TARGET RemoteZynAddSubFx
		POST_BUILD
		COMMAND "${STRIP_COMMAND}" "$<TARGET_FILE:RemoteZynAddSubFx>"
		VERBATIM
		COMMAND_EXPAND_LISTS
	)
ENDIF(LMMS_BUILD_WIN32)
