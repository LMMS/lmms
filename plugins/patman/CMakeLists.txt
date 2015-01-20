INCLUDE(BuildPlugin)

BUILD_PLUGIN(patman patman.cpp patman.h MOCFILES patman.h
	QRESOURCES GLOB *.png)
