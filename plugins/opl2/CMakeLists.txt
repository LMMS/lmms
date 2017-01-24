INCLUDE(BuildPlugin)

# Enable C++11 for CXXFLAGS only
REMOVE_DEFINITIONS(-std=c++0x)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

BUILD_PLUGIN(OPL2 opl2instrument.cpp opl2instrument.h opl.h fmopl.c fmopl.h temuopl.cpp temuopl.h MOCFILES opl2instrument.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
