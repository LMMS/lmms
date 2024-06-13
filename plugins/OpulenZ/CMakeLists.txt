include(BuildPlugin)

add_library(adplug STATIC
	adplug/src/fmopl.c
	adplug/src/temuopl.cpp
)
target_include_directories(adplug PUBLIC adplug/src)
set_target_properties(adplug PROPERTIES SYSTEM TRUE)

build_plugin(opulenz
	OpulenZ.cpp
	OpulenZ.h
	MOCFILES OpulenZ.h
	EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png"
)
target_link_libraries(opulenz adplug)
