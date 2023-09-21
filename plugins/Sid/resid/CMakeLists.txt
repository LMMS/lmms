add_library(resid OBJECT 
    resid/dac.cc
    resid/envelope.cc
    resid/extfilt.cc
    resid/filter8580new.cc
    resid/filter.cc
    resid/pot.cc
    resid/sid.cc
    resid/voice.cc
    resid/wave.cc
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resid/siddefs.h.in ${CMAKE_CURRENT_SOURCE_DIR}/resid/siddefs.h @ONLY)
target_include_directories(resid PUBLIC resid)