INCLUDE(BuildPlugin)

BUILD_PLUGIN(organic organic.cpp organic.h MOCFILES organic.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
