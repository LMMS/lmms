INCLUDE(BuildPlugin)

BUILD_PLUGIN(flanger FlangerEffect.cpp FlangerControls.cpp FlangerControlsDialog.cpp Noise.cpp QuadratureLfo.cpp MonoDelay.cpp MOCFILES FlangerControls.h EMBEDDED_RESOURCES artwork.png)
