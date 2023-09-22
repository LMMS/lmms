# These are the defaults
set(RESID_INLINING 1)
set(RESID_INLINE inline)
set(RESID_BRANCH_HINTS 1)
set(NEW_8580_FILTER 1)

set(HAVE_BOOL 1)
set(HAVE_LOG1P 1)

if (CMAKE_CXX_COMPILER_ID MATCHES "GCC,Clang")
    set(HAVE_BUILTIN_EXPECT 1)
else()
    set(HAVE_BUILTIN_EXPECT 0)
endif()

configure_file(resid/siddefs.h.in resid/siddefs.h @ONLY)
configure_file(resid/siddefs.h.in ${CMAKE_CURRENT_SOURCE_DIR}/resid/siddefs.h @ONLY)

add_library(resid OBJECT
    resid/sid.cc
    resid/voice.cc
    resid/wave.cc
    resid/envelope.cc
    resid/filter8580new.cc
    resid/dac.cc
    resid/extfilt.cc
    resid/pot.cc
    resid/version.cc
)

set(RESID_WAVE_DATA
    wave6581_PST
    wave6581_PS_
    wave6581_P_T
    wave6581__ST
    wave8580_PST
    wave8580_PS_
    wave8580_P_T
    wave8580__ST
)

find_package(Perl)
if(${PERL_FOUND})
    set(RESID_SAMP2SRC_SCRIPT samp2src.pl)
    foreach(WAVE_DATA ${RESID_WAVE_DATA})
        add_custom_command(
            OUTPUT
                ${CMAKE_CURRENT_SOURCE_DIR}/resid/${WAVE_DATA}.h
            COMMAND
                ${PERL_EXECUTABLE} 
                ${CMAKE_CURRENT_SOURCE_DIR}/resid/${RESID_SAMP2SRC_SCRIPT}
                ${CMAKE_CURRENT_SOURCE_DIR}/resid/${WAVE_DATA}
                ${CMAKE_CURRENT_SOURCE_DIR}/resid/${WAVE_DATA}.dat
                ${CMAKE_CURRENT_SOURCE_DIR}/resid/${WAVE_DATA}.h
                VERBATIM
            COMMENT "Generating resid wave data"
        )
        target_sources(resid PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/resid/${WAVE_DATA}.h)
    endforeach()
else()
    message(SEND_ERROR "Cannot build resid - Perl not found")
endif()

# Used the version from the previous submodule, this might not be accurate
set(SRC_VER 0.16)
set(MAJOR_VER 5)
set(MINOR_VER 0)
set(PATCH_VER 0)
target_compile_definitions(resid PUBLIC VERSION="${MAJOR_VER}.${MINOR_VER}.${PATCH_VER}")
target_include_directories(resid PUBLIC resid)