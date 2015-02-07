INCLUDE(BuildPlugin)

BUILD_PLUGIN(nes
	Nes.cpp Nes.h MOCFILES Nes.h
	QRESOURCES GLOB *.png
)
