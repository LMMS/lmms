INCLUDE(BuildPlugin)

BUILD_PLUGIN(dispersion Dispersion.cpp DispersionControls.cpp DispersionControlDialog.cpp MOCFILES DispersionControls.h DispersionControlDialog.h EMBEDDED_RESOURCES logo.svg)
