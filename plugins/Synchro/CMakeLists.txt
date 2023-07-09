INCLUDE(BuildPlugin)

BUILD_PLUGIN(synchro SynchroSynth.cpp SynchroSynth.h MOCFILES SynchroSynth.h
	EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")

TARGET_LINK_LIBRARIES(synchro hiir)