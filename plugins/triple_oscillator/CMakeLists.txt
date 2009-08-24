INCLUDE(BuildPlugin)

BUILD_PLUGIN(tripleoscillator TripleOscillator.cpp TripleOscillator.h MOCFILES TripleOscillator.h EMBEDDED_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.png)
