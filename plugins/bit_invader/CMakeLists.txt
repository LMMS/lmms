INCLUDE(BuildPlugin)

BUILD_PLUGIN(bitinvader bit_invader.cpp bit_invader.h MOCFILES bit_invader.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
