INCLUDE(BuildPlugin)

BUILD_PLUGIN(monstro Monstro.cpp Monstro.h MOCFILES Monstro.h
	QRESOURCES GLOB *.png
)
