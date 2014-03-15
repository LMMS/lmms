INCLUDE(BuildPlugin)

BUILD_PLUGIN(amplifier Amplifier.cpp AmplifierControls.cpp AmplifierControlDialog.cpp MOCFILES AmplifierControls.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
