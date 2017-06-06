INCLUDE(BuildPlugin)

# Enable C++11 for CXXFLAGS only
# This is needed since this plugin uses C. Otherwise Travis fails
REMOVE_DEFINITIONS(-std=c++0x)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

BUILD_PLUGIN(
	reverbsc
	ReverbSC.cpp 
	ReverbSCControls.cpp 
	ReverbSCControlDialog.cpp 
	base.c
	revsc.c
	base.h
	revsc.h
	dcblock.h
	dcblock.c
	ReverbSC.h
	MOCFILES 
	ReverbSCControls.h 
	ReverbSCControlDialog.h 
	base.h
	revsc.h
	dcblock.h
	EMBEDDED_RESOURCES artwork.png logo.png
)
