INCLUDE(BuildPlugin)

BUILD_PLUGIN(midiimport MidiImport.cpp MidiImport.h MOCFILES MidiImport.h)
TARGET_LINK_LIBRARIES(midiimport PortSMF)
