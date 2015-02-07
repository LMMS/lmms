INCLUDE(BuildPlugin)

BUILD_PLUGIN(dualfilter
	DualFilter.cpp DualFilterControls.cpp DualFilterControlDialog.cpp
	MOCFILES DualFilterControls.h DualFilterControlDialog.h
	QRESOURCES artwork.png)
