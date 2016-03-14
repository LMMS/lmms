INCLUDE(BuildPlugin)

# Enable C++11
ADD_DEFINITIONS(-std=c++0x)

BUILD_PLUGIN(midiexport MidiExport.cpp MidiExport.h MidiFile.hpp
		MOCFILES MidiExport.h)
