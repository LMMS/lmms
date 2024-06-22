include(BuildPlugin)
build_plugin(granularpitchshifter
	GranularPitchShifterEffect.cpp
	GranularPitchShifterControls.cpp
	GranularPitchShifterControlDialog.cpp
	MOCFILES
	GranularPitchShifterControls.h
	GranularPitchShifterControlDialog.h
	EMBEDDED_RESOURCES *.png)
