INCLUDE(BuildPlugin)

BUILD_PLUGIN(wavescanner WaveScanner.cpp WaveScanner.h MOCFILES WaveScanner.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
