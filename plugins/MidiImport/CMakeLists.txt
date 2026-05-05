INCLUDE(BuildPlugin)

IF (NOT MSVC)
	# portsmf raises these warnings, which will prevent compilation with -Werror.
	ADD_COMPILE_OPTIONS(-Wno-error=unknown-pragmas -Wno-error=maybe-uninitialized -Wno-error=restrict)
ENDIF()

# For now, portsmf files are included directly in the plugin.
INCLUDE_DIRECTORIES("portsmf/include")
BUILD_PLUGIN(midiimport MidiImport.cpp MidiImport.h
	portsmf/include/allegro.h portsmf/src/allegro.cpp
	portsmf/include/mfmidi.h portsmf/src/mfmidi.cpp
	portsmf/include/strparse.h portsmf/src/strparse.cpp
	# portsmf/src/allegrosmfwr.cpp portsmf/src/allegrowr.cpp
	portsmf/src/allegrosmfrd.cpp portsmf/src/allegrord.cpp
	portsmf/src/algsmfrd_internal.h portsmf/src/algsmfrd_internal.h
	MOCFILES MidiImport.h)
