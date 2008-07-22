INCLUDE(BuildPlugin)

BUILD_PLUGIN(tripleoscillator triple_oscillator.cpp triple_oscillator.h MOCFILES triple_oscillator.h EMBEDDED_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.png)
