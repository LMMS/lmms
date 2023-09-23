INCLUDE(BuildPlugin)

if(NOT ${LMMS_HAVE_PERL})
	message(STATUS "Cannot build Sid plugin, Perl not found")
	return()
endif()

BUILD_PLUGIN(sid
	SidInstrument.cpp
	SidInstrument.h
	MOCFILES SidInstrument.h
	EMBEDDED_RESOURCES *.png)

add_subdirectory(resid)
target_link_libraries(sid resid)
