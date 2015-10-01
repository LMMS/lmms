INCLUDE(BuildPlugin)

INCLUDE_DIRECTORIES(unrtf)

# Enable C++11
ADD_DEFINITIONS(-std=c++0x)

IF(LMMS_BUILD_CLANG)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
ENDIF()

BUILD_PLUGIN(flpimport FlpImport.cpp unrtf.cpp FlpImport.h)

