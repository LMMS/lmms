INCLUDE(BuildPlugin)

BUILD_PLUGIN(bitcrush
	Bitcrush.cpp BitcrushControls.cpp BitcrushControlDialog.cpp
	MOCFILES BitcrushControls.h BitcrushControlDialog.h
	QRESOURCES artwork.png)
