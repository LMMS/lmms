INCLUDE(BuildPlugin)

include_directories(resid/resid)
add_subdirectory(resid)

BUILD_PLUGIN(sid
	SidInstrument.cpp
	SidInstrument.h
	$<TARGET_OBJECTS:resid>
	MOCFILES SidInstrument.h
	EMBEDDED_RESOURCES *.png)
