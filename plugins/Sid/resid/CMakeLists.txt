# These are the defaults
set(RESID_INLINING 1)
set(RESID_INLINE inline)
set(RESID_BRANCH_HINTS 1)
set(NEW_8580_FILTER 0)

set(HAVE_BOOL 1)
set(HAVE_LOG1P 1)

if(CMAKE_CXX_COMPILER_ID MATCHES "GCC|Clang")
	set(HAVE_BUILTIN_EXPECT 1)
else()
	set(HAVE_BUILTIN_EXPECT 0)
endif()

configure_file(resid/siddefs.h.in resid/siddefs.h @ONLY)

add_library(resid_objects OBJECT
	resid/sid.cc
	resid/voice.cc
	resid/wave.cc
	resid/envelope.cc
	resid/filter.cc
	resid/dac.cc
	resid/extfilt.cc
	resid/pot.cc
	resid/version.cc
)

target_include_directories(resid_objects PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/resid"
	"${CMAKE_CURRENT_BINARY_DIR}/resid"
)
target_compile_definitions(resid_objects PUBLIC VERSION="1.0")

set(RESID_WAVES
	wave6581_PST
	wave6581_PS_
	wave6581_P_T
	wave6581__ST
	wave8580_PST
	wave8580_PS_
	wave8580_P_T
	wave8580__ST
)

set(RESID_SAMP2SRC_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/resid/samp2src.pl)
foreach(WAVE_DATA IN LISTS RESID_WAVES)
	set(WAVE_DATA_IN ${CMAKE_CURRENT_SOURCE_DIR}/resid/${WAVE_DATA}.dat)
	set(WAVE_SRC_OUT ${CMAKE_CURRENT_BINARY_DIR}/resid/${WAVE_DATA}.h)
	set(WAVE_COMMAND
		"${PERL_EXECUTABLE}"
		"${RESID_SAMP2SRC_SCRIPT}"
		"${WAVE_DATA}"
		"${WAVE_DATA_IN}"
		"${WAVE_SRC_OUT}"
	)
	add_custom_command(OUTPUT ${WAVE_SRC_OUT} COMMAND ${WAVE_COMMAND} VERBATIM)
	target_sources(resid_objects PUBLIC ${WAVE_SRC_OUT})
endforeach()

# TODO CMake 3.12: Use target_link_libraries() to propagate usage requirements directly to sid plugin
add_library(resid INTERFACE)

target_sources(resid INTERFACE $<TARGET_OBJECTS:resid_objects>)

get_target_property(resid_includes resid_objects INCLUDE_DIRECTORIES)
target_include_directories(resid INTERFACE ${resid_includes})
