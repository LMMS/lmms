INCLUDE(BuildPlugin)

BUILD_PLUGIN(sfxr
	sfxr.cpp sfxr.h MOCFILES sfxr.h
	QRESOURCES GLOB *.png
)
