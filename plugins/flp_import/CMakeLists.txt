INCLUDE(BuildPlugin)

INCLUDE_DIRECTORIES(unrtf)

BUILD_PLUGIN(flpimport FlpImport.cpp unrtf.cpp FlpImport.h)
