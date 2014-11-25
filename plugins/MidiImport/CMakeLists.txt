INCLUDE(BuildPlugin)

BUILD_PLUGIN(midiimport MidiImport.cpp MidiImport.h
	portsmf/allegro.cpp portsmf/allegro.h portsmf/allegrosmfwr.cpp
	portsmf/allegrord.cpp portsmf/allegrowr.cpp portsmf/allegrosmfrd.cpp
	portsmf/mfmidi.cpp portsmf/mfmidi.h portsmf/strparse.cpp
	portsmf/strparse.h portsmf/algrd_internal.h portsmf/algsmfrd_internal.h
	portsmf/trace.h MOCFILES MidiImport.h)
