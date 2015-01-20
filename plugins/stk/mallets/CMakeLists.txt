INCLUDE(BuildPlugin)
INCLUDE_DIRECTORIES("${STK_INCLUDE_DIR}")
LINK_LIBRARIES(${STK_LIBRARY})
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
BUILD_PLUGIN(malletsstk
	mallets.cpp mallets.h MOCFILES mallets.h
	QRESOURCES artwork.png logo.png
)
