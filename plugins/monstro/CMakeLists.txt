INCLUDE(BuildPlugin)

BUILD_PLUGIN(monstro Monstro.cpp Monstro.h MOCFILES Monstro.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
