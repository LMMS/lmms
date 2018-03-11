INCLUDE(BuildPlugin)
INCLUDE_DIRECTORIES(game-music-emu/gme)

BUILD_PLUGIN(freeboy
	FreeBoy.cpp
	FreeBoy.h
	Gb_Apu_Buffer.cpp
	Gb_Apu_Buffer.h
	game-music-emu/gme/Gb_Apu.cpp
	game-music-emu/gme/Gb_Apu.h
	game-music-emu/gme/Gb_Oscs.cpp
	game-music-emu/gme/Blip_Buffer.cpp
	game-music-emu/gme/Gb_Oscs.h
	game-music-emu/gme/blargg_common.h
	game-music-emu/gme/Blip_Buffer.h
	game-music-emu/gme/Multi_Buffer.cpp
	game-music-emu/gme/blargg_source.h
	game-music-emu/gme/Multi_Buffer.h
	MOCFILES FreeBoy.h
	EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png"
)
