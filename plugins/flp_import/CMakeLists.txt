INCLUDE(BuildPlugin)

INCLUDE_DIRECTORIES(unrtf)

LINK_LIBRARIES(${ZIP_LIBRARIES})

BUILD_PLUGIN(flpimport FlpImport.cpp unrtf.cpp FlpImport.h)
