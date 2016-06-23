INCLUDE(BuildPlugin)

# Enable C++11
ADD_DEFINITIONS(-std=c++0x)

IF(LMMS_BUILD_APPLE)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
ENDIF()

BUILD_PLUGIN(midiexport MidiExport.cpp MidiExport.h MidiFile.hpp
		MOCFILES MidiExport.h)
