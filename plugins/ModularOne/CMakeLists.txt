INCLUDE(BuildPlugin)

BUILD_PLUGIN(ModularOne 
    ModularOne.cpp 
    ModularOne.h 
    MOCFILES ModularOne.h 
    EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png"
)
