INCLUDE(BuildPlugin)

BUILD_PLUGIN(midiexport MidiExport.cpp MidiExport.h MidiFile.cpp MidiFile.h
		MOCFILES MidiExport.h)
