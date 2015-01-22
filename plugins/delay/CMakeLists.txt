INCLUDE(BuildPlugin)

BUILD_PLUGIN(delay delayeffect.cpp delaycontrols.cpp delaycontrolsdialog.cpp lfo.cpp stereodelay.cpp
	MOCFILES delaycontrols.h
	QRESOURCES artwork.png)
