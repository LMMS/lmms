INCLUDE(BuildPlugin)

BUILD_PLUGIN(waveshaper2 waveshaper2.cpp waveshaper2_controls.cpp waveshaper2_control_dialog.cpp MOCFILES waveshaper2_controls.h waveshaper2_control_dialog.h EMBEDDED_RESOURCES *.png)
