INCLUDE(BuildPlugin)

BUILD_PLUGIN(delay DelayEffect.cpp DelayControls.cpp DelayControlsDialog.cpp Lfo.cpp StereoDelay.cpp  MOCFILES DelayControls.h DelayControlsDialog.h ../Eq/EqFader.h EMBEDDED_RESOURCES artwork.png)
