INCLUDE(BuildPlugin)

BUILD_PLUGIN(bitinvader BitInvader.cpp BitInvader.h MOCFILES BitInvader.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
