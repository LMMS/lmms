INCLUDE(BuildPlugin)

BUILD_PLUGIN(
	flanger FlangerEffect.cpp FlangerControls.cpp FlangerControlsDialog.cpp MonoDelay.cpp
	MOCFILES FlangerControls.h FlangerControlsDialog.h
	EMBEDDED_RESOURCES artwork.svg logo.svg
)
