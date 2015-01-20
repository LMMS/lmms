INCLUDE(BuildPlugin)

BUILD_PLUGIN(bitinvader
	bit_invader.cpp bit_invader.h MOCFILES bit_invader.h
	QRESOURCES GLOB *.png)
