INCLUDE(BuildPlugin)

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
	EMBEDDED_RESOURCES artwork.svg logo.svg
)
