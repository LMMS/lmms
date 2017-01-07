INCLUDE(BuildPlugin)

BUILD_PLUGIN(midiexport MidiExport.cpp MidiExport.h MidiFile.hpp
		MOCFILES MidiExport.h)
