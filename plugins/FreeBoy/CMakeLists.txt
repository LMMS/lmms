include(BuildPlugin)

add_library(gme STATIC
	game-music-emu/gme/Gb_Apu.cpp
	game-music-emu/gme/Gb_Oscs.cpp
	game-music-emu/gme/Blip_Buffer.cpp
	game-music-emu/gme/Multi_Buffer.cpp
)
target_include_directories(gme PUBLIC game-music-emu/gme)
set_target_properties(gme PROPERTIES SYSTEM TRUE)

build_plugin(freeboy
	FreeBoy.cpp
	FreeBoy.h
	GbApuWrapper.cpp
	GbApuWrapper.h
	MOCFILES FreeBoy.h
	EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png"
)
target_link_libraries(freeboy gme)
