INCLUDE(BuildPlugin)

BUILD_PLUGIN(dualfilter DualFilter.cpp DualFilterControls.cpp DualFilterControlDialog.cpp MOCFILES DualFilterControls.h DualFilterControlDialog.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
