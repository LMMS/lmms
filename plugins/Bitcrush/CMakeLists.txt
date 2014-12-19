INCLUDE(BuildPlugin)

BUILD_PLUGIN(bitcrush Bitcrush.cpp BitcrushControls.cpp BitcrushControlDialog.cpp MOCFILES BitcrushControls.h BitcrushControlDialog.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
