#checking include/library paths
message(STATUS "Checking Include Path" $ENV{CMAKE_INCLUDE_PATH} ${CMAKE_INCLUDE_PATH})
message(STATUS "Checking Library Path" $ENV{CMAKE_LIBRARY_PATH} ${CMAKE_LIBRARY_PATH})

#Dependency check
find_package(PkgConfig REQUIRED)
find_package(zlib REQUIRED)
pkg_check_modules(FFTW REQUIRED fftw3)
pkg_check_modules(MXML REQUIRED mxml)
find_package(Threads   REQUIRED)
find_package(OSS)
find_package(Alsa)
pkg_check_modules(JACK jack)
pkg_check_modules(PORTAUDIO portaudio-2.0>=19)
set(FLTK_SKIP_OPENGL true)
pkg_check_modules(NTK ntk)
pkg_check_modules(NTK_IMAGES ntk_images)
find_package(FLTK)
find_package(OpenGL) #for FLTK
find_package(CxxTest)
if(CXXTEST_FOUND)
    set(CXXTEST_USE_PYTHON TRUE)
endif()
# lash
pkg_search_module(LASH lash-1.0)
mark_as_advanced(LASH_LIBRARIES)
pkg_search_module(DSSI dssi>=0.9.0)
mark_as_advanced(DSSI_LIBRARIES)
pkg_search_module(LIBLO liblo>=0.26)
mark_as_advanced(LIBLO_LIBRARIES)

CHECK_FUNCTION_EXISTS(sched_setscheduler HAVE_SCHEDULER)

execute_process(COMMAND echo fistpl 0
                COMMAND as -
                ERROR_VARIABLE AVOID_ASM)

######### Settings ###########
# NOTE: These cache variables should normally not be changed in this
# file, but either in in CMakeCache.txt before compile, or by passing
# parameters directly into cmake using the -D flag.
SET (GuiModule fltk CACHE STRING "GUI module, either fltk, ntk or off")
SET (CompileTests ${CXXTEST_FOUND} CACHE BOOL "whether tests should be compiled in or not")
SET (AlsaEnable ${ALSA_FOUND} CACHE BOOL
    "Enable support for Advanced Linux Sound Architecture")
SET (JackEnable ${JACK_FOUND} CACHE BOOL
    "Enable support for JACK Audio Connection toolKit")
SET (OssEnable ${OSS_FOUND} CACHE BOOL
    "Enable support for Open Sound System")
SET (PaEnable ${PORTAUDIO_FOUND} CACHE BOOL
    "Enable support for Port Audio System")
SET (LashEnable ${LASH_FOUND} CACHE BOOL
    "Enable LASH Audio Session Handler")
SET (DssiEnable ${DSSI_FOUND} CACHE BOOL
    "Enable DSSI Plugin compilation")
SET (LibloEnable ${LIBLO_FOUND} CACHE BOOL
    "Enable Liblo")

# Now, handle the incoming settings and set define flags/variables based
# on this

# Add version information
add_definitions(-DVERSION="${VERSION}")

message(STATUS "Building on a '${CMAKE_SYSTEM_NAME}' System")

if(NOT "Darwin" STREQUAL ${CMAKE_SYSTEM_NAME})
    # Add scheduler function existance info (OSX compatiability)
    add_definitions(-DHAVE_SCHEDULER=${HAVE_SCHEDULER})
endif()


# Give a good guess on the best Input/Output default backends
if (JackEnable)
    SET (DefaultOutput jack CACHE STRING
        "Default Output module: [null, alsa, oss, jack, portaudio]")
    # Override with perhaps more helpful midi backends
    if (AlsaEnable)
        SET (DefaultInput alsa CACHE STRING
            "Default Input module: [null, alsa, oss, jack]")
    elseif (OssEnable)
        SET (DefaultInput oss CACHE STRING
            "Default Input module: [null, alsa, oss, jack]")
    else ()
        SET (DefaultInput jack CACHE STRING
            "Default Input module: [null, alsa, oss, jack]")
    endif ()
elseif (AlsaEnable)
    SET (DefaultOutput alsa CACHE STRING
        "Default Output module: [null, alsa, oss, jack, portaudio]")
    SET (DefaultInput alsa CACHE STRING
        "Default Input module: [null, alsa, oss, jack]")
elseif (OssEnable)
    SET (DefaultOutput oss CACHE STRING
        "Default Output module: [null, alsa, oss, jack, portaudio]")
    SET (DefaultInput oss CACHE STRING
        "Default Input module: [null, alsa, oss, jack]")
else()
    SET (DefaultOutput null CACHE STRING
        "Default Output module: [null, alsa, oss, jack, portaudio]")
    SET (DefaultInput null CACHE STRING
        "Default Input module: [null, alsa, oss, jack]")
endif()



if (GuiModule STREQUAL qt AND QT_FOUND)
	set (QtGui TRUE)
elseif(GuiModule STREQUAL ntk AND NTK_FOUND)
	set (NtkGui TRUE)
elseif(GuiModule STREQUAL fltk AND FLTK_FOUND)
	set (FltkGui TRUE)
elseif(GuiModule STREQUAL off)
        add_definitions(-DDISABLE_GUI)
else  ()
        set (GuiModule off CACHE STRING "GUI module, either fltk, qt or off")
        add_definitions(-DDISABLE_GUI)
	message(STATUS "GUI module defaulting to off")
endif()


#Build Flags
option (BuildForAMD_X86_64 "Build for AMD x86_64 system" OFF)
option (BuildForCore2_X86_64 "Build for Intel Core2 x86_64 system" OFF)
option (BuildForDebug "Include gdb debugging support" OFF)

set(CMAKE_BUILD_TYPE "Release")

set (BuildOptions_x86_64AMD
    "-O3 -march=athlon64 -m64 -Wall -ffast-math -fno-finite-math-only -fomit-frame-pointer"
  CACHE STRING "X86_64 compiler options"
)

set (BuildOptions_X86_64Core2
    "-O3 -march=core2 -m64 -Wall -ffast-math -fno-finite-math-only -fomit-frame-pointer"
  CACHE STRING "X86_64 compiler options"
)

set (BuildOptionsBasic
    "-O3 -msse -msse2 -mfpmath=sse -ffast-math -fomit-frame-pointer"
    CACHE STRING "basic X86 complier options"
)

set (BuildOptionsDebug
    "-O0 -g3 -ggdb -Wall -Wpointer-arith" CACHE STRING "Debug build flags")

########### Settings dependant code ###########
# From here on, the setting variables have  been prepared so concentrate
# on the actual compiling.

if(AlsaEnable)
	list(APPEND AUDIO_LIBRARIES ${ASOUND_LIBRARY})
	list(APPEND AUDIO_LIBRARY_DIRS ${ASOUND_LIBRARY_DIRS})
	add_definitions(-DALSA=1)
endif(AlsaEnable)

if(JackEnable)
	list(APPEND AUDIO_LIBRARIES ${JACK_LIBRARIES})
	list(APPEND AUDIO_LIBRARY_DIRS ${JACK_LIBRARY_DIRS})
	add_definitions(-DJACK=1)
endif(JackEnable)

if(OssEnable)
	add_definitions(-DOSS=1)
endif(OssEnable)

if(PaEnable)
	include_directories(${PORTAUDIO_INCLUDE_DIR})
	add_definitions(-DPORTAUDIO=1)
	list(APPEND AUDIO_LIBRARIES ${PORTAUDIO_LIBRARIES})
	list(APPEND AUDIO_LIBRARY_DIRS ${PORTAUDIO_LIBRARY_DIRS})
endif()

if (CompileTests)
	ENABLE_TESTING()
endif()

if(LashEnable)
	include_directories(${LASH_INCLUDE_DIRS})
	add_definitions(-DLASH=1)
	list(APPEND AUDIO_LIBRARIES ${LASH_LIBRARIES})
	list(APPEND AUDIO_LIBRARY_DIRS ${LASH_LIBRARY_DIRS})
	message(STATUS "Compiling with lash")
endif()
if(LibloEnable)
	include_directories(${LIBLO_INCLUDE_DIRS})
	add_definitions(-DUSE_NSM=1)
	list(APPEND AUDIO_LIBRARIES ${LIBLO_LIBRARIES})
	list(APPEND AUDIO_LIBRARY_DIRS ${LIBLO_LIBRARY_DIRS})
	message(STATUS "Compiling with liblo")
endif()

# other include directories
include_directories(${ZLIB_INCLUDE_DIRS} ${MXML_INCLUDE_DIRS})

add_definitions(
	 -g #TODO #todo put in a better location
	 -Wall
	 -Wextra
	 )
if(NOT AVOID_ASM)
	message(STATUS "Compiling with x86 opcode support")
    add_definitions(-DASM_F2I_YES)
endif()

if (BuildForDebug)
	set (CMAKE_BUILD_TYPE "Debug")
	set (CMAKE_CXX_FLAGS_DEBUG ${BuildOptionsDebug})
	message (STATUS "Building for ${CMAKE_BUILD_TYPE}, flags: ${CMAKE_CXX_FLAGS_DEBUG}")
else (BuildForDebug)
	set (CMAKE_BUILD_TYPE "Release")
	if (BuildForAMD_X86_64)
		set (CMAKE_CXX_FLAGS_RELEASE ${BuildOptions_x86_64AMD})
	else (BuildForAMD_X86_64)
		if (BuildForCore2_X86_64)
			set (CMAKE_CXX_FLAGS_RELEASE ${BuildOptions_X86_64Core2})
		else (BuildForCore2_X86_64)
			set (CMAKE_CXX_FLAGS_RELEASE ${BuildOptionsBasic})
		endif (BuildForCore2_X86_64)
	endif (BuildForAMD_X86_64)
	message (STATUS "Building for ${CMAKE_BUILD_TYPE}, flags: ${CMAKE_CXX_FLAGS_RELEASE}")
endif (BuildForDebug)

add_definitions(-fPIC)

if(FLTK_FOUND)
	mark_as_advanced(FORCE FLTK_BASE_LIBRARY)
	mark_as_advanced(FORCE FLTK_CONFIG_SCRIPT)
	mark_as_advanced(FORCE FLTK_DIR)
	mark_as_advanced(FORCE FLTK_FLUID_EXECUTABLE)
	mark_as_advanced(FORCE FLTK_FORMS_LIBRARY)
	mark_as_advanced(FORCE FLTK_GL_LIBRARY)
	mark_as_advanced(FORCE FLTK_IMAGES_LIBRARY)
	mark_as_advanced(FORCE FLTK_INCLUDE_DIR)
	mark_as_advanced(FORCE FLTK_MATH_LIBRARY)
endif(FLTK_FOUND)

if(NTK_FOUND)
	mark_as_advanced(FORCE NTK_BASE_LIBRARY)
	mark_as_advanced(FORCE NTK_CONFIG_SCRIPT)
	mark_as_advanced(FORCE NTK_DIR)
	mark_as_advanced(FORCE FLTK_FLUID_EXECUTABLE)
	mark_as_advanced(FORCE NTK_FORMS_LIBRARY)
	mark_as_advanced(FORCE NTK_GL_LIBRARY)
	mark_as_advanced(FORCE NTK_IMAGES_LIBRARY)
	mark_as_advanced(FORCE NTK_INCLUDE_DIR)
	mark_as_advanced(FORCE NTK_MATH_LIBRARY)
endif(NTK_FOUND)

if(FltkGui)
	#UGLY WORKAROUND
	find_program (FLTK_CONFIG fltk-config)
	if (FLTK_CONFIG)
		execute_process (COMMAND ${FLTK_CONFIG} --use-images --ldflags OUTPUT_VARIABLE FLTK_LDFLAGS)
		string(STRIP ${FLTK_LDFLAGS} FLTK_LIBRARIES)
	endif()

	message(STATUS ${FLTK_LDFLAGS})


	set(GUI_LIBRARIES ${FLTK_LIBRARIES} ${FLTK_LIBRARIES} ${OPENGL_LIBRARIES} zynaddsubfx_gui)

	add_definitions(-DFLTK_GUI)
	message(STATUS "Will build FLTK gui")

	include_directories(
			${FLTK_INCLUDE_DIR}
			"${CMAKE_CURRENT_SOURCE_DIR}/UI"
			"${CMAKE_CURRENT_BINARY_DIR}/UI"
			)

	add_subdirectory(UI)
endif()

if(NtkGui)

        find_program( FLTK_FLUID_EXECUTABLE ntk-fluid)
        
	message(STATUS ${NTK_LDFLAGS} ${NTK_IMAGES_LDFLAGS})

	set(GUI_LIBRARIES ${NTK_LIBRARIES} ${NTK_IMAGES_LIBRARIES} ${OPENGL_LIBRARIES} zynaddsubfx_gui)

	add_definitions(-DNTK_GUI)

	message(STATUS "Will build NTK gui")

	include_directories(
			${NTK_INCLUDE_DIRS}
			"${CMAKE_CURRENT_SOURCE_DIR}/UI"
			"${CMAKE_CURRENT_BINARY_DIR}/UI"
			)

	add_subdirectory(UI)
endif()

########### General section ##############
# Following this should be only general compilation code, and no mention
# of module-specific variables

link_directories(${AUDIO_LIBRARY_DIRS} ${ZLIB_LIBRARY_DIRS} ${FFTW_LIBRARY_DIRS} ${MXML_LIBRARY_DIRS} ${FLTK_LIBRARY_DIRS} ${NTK_LIBRARY_DIRS})

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	)



set(NONGUI_LIBRARIES
	zynaddsubfx_misc
	zynaddsubfx_synth
	zynaddsubfx_effect
	zynaddsubfx_params
	zynaddsubfx_dsp
	zynaddsubfx_nio
	)

add_subdirectory(Misc)
add_subdirectory(Synth)
add_subdirectory(Effects)
add_subdirectory(Params)
add_subdirectory(DSP)
add_subdirectory(Nio)

add_library(zynaddsubfx_core STATIC
	${zynaddsubfx_dsp_SRCS}
	${zynaddsubfx_effect_SRCS}
	${zynaddsubfx_misc_SRCS}
	${zynaddsubfx_params_SRCS}
	${zynaddsubfx_synth_SRCS}
	)

target_link_libraries(zynaddsubfx_core
	${ZLIB_LIBRARIES}
	${FFTW_LIBRARIES}
	${MXML_LIBRARIES}
	${OS_LIBRARIES}
	pthread)

if(CompileTests)
	add_subdirectory(Tests)
endif(CompileTests)

message(STATUS "using link directories: ${AUDIO_LIBRARY_DIRS} ${ZLIB_LIBRARY_DIRS} ${FFTW_LIBRARY_DIRS} ${MXML_LIBRARY_DIRS} ${FLTK_LIBRARY_DIRS}")


add_executable(zynaddsubfx main.cpp)

target_link_libraries(zynaddsubfx
    zynaddsubfx_core
	zynaddsubfx_nio
	${GUI_LIBRARIES}
	${NIO_LIBRARIES}
	${AUDIO_LIBRARIES}
	)

if (DssiEnable)
	add_library(zynaddsubfx_dssi SHARED
		Output/DSSIaudiooutput.cpp
		)

	target_link_libraries(zynaddsubfx_dssi
        zynaddsubfx_core
		${OS_LIBRARIES}
		)
	if (${CMAKE_SIZEOF_VOID_P} EQUAL "8")
		install(TARGETS zynaddsubfx_dssi LIBRARY DESTINATION lib64/dssi/)
	else ()
		install(TARGETS zynaddsubfx_dssi LIBRARY DESTINATION lib/dssi/)
	endif ()
endif()

message(STATUS "Link libraries: ${ZLIB_LIBRARY} ${FFTW_LIBRARY} ${MXML_LIBRARIES} ${AUDIO_LIBRARIES} ${OS_LIBRARIES}")
install(TARGETS zynaddsubfx
	RUNTIME DESTINATION bin
	)

if(NtkGui)
install(DIRECTORY ../pixmaps DESTINATION share/zynaddsubfx)
add_definitions(-DPIXMAP_PATH="${CMAKE_INSTALL_PREFIX}/share/zynaddsubfx/pixmaps/")
add_definitions(-DSOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
endif(NtkGui)

include(CTest)
