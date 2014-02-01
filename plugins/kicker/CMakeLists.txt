INCLUDE(BuildPlugin)

BUILD_PLUGIN(kicker kicker.cpp kicker.h MOCFILES kicker.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
