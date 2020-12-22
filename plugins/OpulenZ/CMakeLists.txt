INCLUDE(BuildPlugin)

# Avoid unused warnings for mididata.h
IF(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable")
ENDIF()

INCLUDE_DIRECTORIES(adplug/src)

BUILD_PLUGIN(opulenz
	OpulenZ.cpp
	OpulenZ.h
	adplug/src/opl.h
	adplug/src/fmopl.c
	adplug/src/fmopl.h
	adplug/src/temuopl.cpp
	adplug/src/temuopl.h
	MOCFILES OpulenZ.h
	EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png"
)
