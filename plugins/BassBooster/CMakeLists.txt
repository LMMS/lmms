INCLUDE(BuildPlugin)

BUILD_PLUGIN(bassbooster
	BassBooster.cpp BassBoosterControls.cpp BassBoosterControlDialog.cpp
	MOCFILES BassBoosterControls.h BassBoosterControlDialog.h
	QRESOURCES artwork.png)
