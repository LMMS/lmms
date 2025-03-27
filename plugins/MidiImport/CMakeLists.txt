INCLUDE(BuildPlugin)

add_library(portsmf INTERFACE)
BUILD_PLUGIN(midiimport MidiImport.cpp MidiImport.h portsmf/include/allegro.h
	MOCFILES MidiImport.h)
