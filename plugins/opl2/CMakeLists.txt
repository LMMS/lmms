INCLUDE(BuildPlugin)

# Enable C++11
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

IF(LMMS_BUILD_APPLE)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
ENDIF()

BUILD_PLUGIN(OPL2 opl2instrument.cpp opl2instrument.h opl.h fmopl.c fmopl.h temuopl.cpp temuopl.h MOCFILES opl2instrument.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
