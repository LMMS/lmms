INCLUDE(BuildPlugin)

BUILD_PLUGIN(disintegrator Disintegrator.cpp DisintegratorControls.cpp DisintegratorControlDialog.cpp MOCFILES DisintegratorControls.h DisintegratorControlDialog.h EMBEDDED_RESOURCES artwork.png logo.png)
