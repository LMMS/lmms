INCLUDE(BuildPlugin)

SET(CMAKE_CXX_FLAGS "-fexceptions -ldrumstick-file")

BUILD_PLUGIN(midiimport MidiImport.cpp smfMidiCC.cpp smfMidiChannel.cpp midiReader.cpp MOCFILES MidiImport.h midiReader.h )
