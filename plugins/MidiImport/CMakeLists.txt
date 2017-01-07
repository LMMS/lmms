INCLUDE(BuildPlugin)

SET(CMAKE_CXX_FLAGS "-fexceptions")

BUILD_PLUGIN(midiimport MidiImport.cpp MidiImport.h
        smfMidiCC.cpp smfMidiCC.h smfMidiChannel.cpp smfMidiChannel.h
        midiReader.cpp midiReader.h
        MOCFILES MidiImport.h)
