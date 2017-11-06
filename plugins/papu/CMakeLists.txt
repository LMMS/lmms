# INCLUDE(CheckSubmodule)
# CHECK_SUBMODULE(game-music-emu/CMakeLists.txt)
INCLUDE(BuildPlugin)
INCLUDE_DIRECTORIES(game-music-emu/gme)

BUILD_PLUGIN(papu
	papu_instrument.cpp
	papu_instrument.h
	Basic_Gb_Apu.cpp # TODO: Replace with subclass
	Basic_Gb_Apu.h   # TODO: Replace with subclass
	# Gb_Apu_Buffer.cpp
	# Gb_Apu_Buffer.h
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
	MOCFILES papu_instrument.h
	EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png"
)
