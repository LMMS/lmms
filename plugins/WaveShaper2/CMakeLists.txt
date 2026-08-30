INCLUDE(BuildPlugin)

BUILD_PLUGIN(waveshaper2
	WaveShaper2.cpp WaveShaper2Controls.cpp WaveShaper2ControlDialog.cpp
	MOCFILES WaveShaper2Controls.h WaveShaper2ControlDialog.h
	EMBEDDED_RESOURCES *.png
)
