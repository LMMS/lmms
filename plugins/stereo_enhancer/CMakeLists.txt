INCLUDE(BuildPlugin)

BUILD_PLUGIN(stereoenhancer stereo_enhancer.cpp stereoenhancer_controls.cpp stereoenhancer_control_dialog.cpp stereo_enhancer.h stereoenhancer_controls.h stereoenhancer_control_dialog.h MOCFILES stereoenhancer_controls.h stereoenhancer_control_dialog.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")

