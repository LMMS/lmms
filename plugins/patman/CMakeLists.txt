INCLUDE(BuildPlugin)

BUILD_PLUGIN(patman patman.cpp patman.h MOCFILES patman.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
