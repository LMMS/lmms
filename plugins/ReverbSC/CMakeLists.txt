INCLUDE(BuildPlugin)

BUILD_PLUGIN(
	reverbsc
	ReverbSC.cpp 
	ReverbSCControls.cpp 
	ReverbSCControlDialog.cpp 
	base.c
	revsc.c
	dcblock.c
	MOCFILES 
	ReverbSC.h
	ReverbSCControls.h 
	ReverbSCControlDialog.h 
	base.h
	revsc.h
	dcblock.h
	EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png"
)
