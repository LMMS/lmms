INCLUDE(BuildPlugin)

BUILD_PLUGIN(waveshaper waveshaper.cpp waveshaper_controls.cpp waveshaper_control_dialog.cpp MOCFILES waveshaper_controls.h waveshaper_control_dialog.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
