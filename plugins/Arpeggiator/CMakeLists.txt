INCLUDE(BuildPlugin)

BUILD_PLUGIN(arpeggiator Arpeggiator.cpp ArpeggiatorControls.cpp ArpeggiatorControlDialog.cpp MOCFILES ArpeggiatorControls.h ArpeggiatorControlDialog.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
