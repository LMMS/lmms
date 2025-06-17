INCLUDE(BuildPlugin)

BUILD_PLUGIN(bitcrush Bitcrush.cpp BitcrushControls.cpp BitcrushControlDialog.cpp MOCFILES BitcrushControls.h BitcrushControlDialog.h EMBEDDED_RESOURCES artwork.svg logo.svg)
