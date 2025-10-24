INCLUDE(BuildPlugin)

BUILD_PLUGIN(midiimport MidiImport.cpp MidiImport.h
	portsmf/include/allegro.h portsmf/src/allegro.cpp
	MOCFILES MidiImport.h)
