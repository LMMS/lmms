INCLUDE(BuildPlugin)

BUILD_PLUGIN(midiexport MidiExport.cpp MidiExport.h MOCFILES MidiExport.h)
TARGET_LINK_LIBRARIES(midiexport PortSMF)
