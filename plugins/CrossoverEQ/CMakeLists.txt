INCLUDE(BuildPlugin)

BUILD_PLUGIN(crossovereq CrossoverEQ.cpp CrossoverEQControls.cpp CrossoverEQControlDialog.cpp MOCFILES CrossoverEQControls.h CrossoverEQControlDialog.h EMBEDDED_RESOURCES artwork.png fader_bg.png fader_empty.png fader_knob2.png logo.png)
