INCLUDE(BuildPlugin)

ADD_SUBDIRECTORY(portsmf)
BUILD_PLUGIN(midiimport MidiImport.cpp MidiImport.h portsmf/include/allegro.h
	MOCFILES MidiImport.h)
