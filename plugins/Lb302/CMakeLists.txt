INCLUDE(BuildPlugin)

# GCC complains about std::hardware_destructive_interference_size used in Hardware.h
IF(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	ADD_COMPILE_OPTIONS(-Wno-interference-size)
ENDIF()

BUILD_PLUGIN(lb302 Lb302.cpp Lb302.h MOCFILES Lb302.h EMBEDDED_RESOURCES artwork.png logo.png)
