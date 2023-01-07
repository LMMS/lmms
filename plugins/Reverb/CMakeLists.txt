INCLUDE(BuildPlugin)

BUILD_PLUGIN(
	Reverb
	Reverb.cpp
	ReverbControls.cpp
	ReverbControlDialog.cpp
	base.c
	revsc.c
	base.h
	revsc.h
	dcblock.h
	dcblock.c
	Reverb.h
	MOCFILES
	ReverbControls.h
	ReverbControlDialog.h
	../Eq/EqFader.h
	EMBEDDED_RESOURCES artwork.png logo.png
)
