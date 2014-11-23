INCLUDE(BuildPlugin)

BUILD_PLUGIN(delay delayeffect.cpp delaycontrols.cpp delaycontrolsdialog.cpp lfo.cpp stereodelay.cpp MOCFILES delaycontrols.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
