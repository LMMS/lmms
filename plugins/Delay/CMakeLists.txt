INCLUDE(BuildPlugin)

BUILD_PLUGIN(delay DelayEffect.cpp DelayControls.cpp DelayControlsDialog.cpp Lfo.cpp StereoDelay.cpp MOCFILES DelayControls.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
