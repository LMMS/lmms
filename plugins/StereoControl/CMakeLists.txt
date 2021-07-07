INCLUDE(BuildPlugin)

BUILD_PLUGIN(stereocontrol StereoControl.cpp StereoControlControls.cpp StereoControlControlDialog.cpp MOCFILES StereoControlControls.h StereoControlControlDialog.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
