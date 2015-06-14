INCLUDE(BuildPlugin)

INCLUDE_DIRECTORIES(unrtf)

ADD_DEFINITIONS(--std=c++0x)
BUILD_PLUGIN(flpimport FlpImport.cpp unrtf.cpp FlpImport.h)
