INCLUDE(BuildPlugin)

BUILD_PLUGIN(flanger flangereffect.cpp flangercontrols.cpp flangercontrolsdialog.cpp noise.cpp quadraturelfo.cpp monodelay.cpp MOCFILES flangercontrols.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
