INCLUDE(BuildPlugin)

BUILD_PLUGIN(waveshaper WaveShaper.cpp WaveShaperControls.cpp WaveShaperControlDialog.cpp MOCFILES WaveShaperControls.h WaveShaperControlDialog.h EMBEDDED_RESOURCES *.png)
