INCLUDE(BuildPlugin)

BUILD_PLUGIN(OPL2
	opl2instrument.cpp opl2instrument.h opl.h fmopl.c fmopl.h temuopl.cpp temuopl.h
	MOCFILES opl2instrument.h
	QRESOURCES GLOB *.png)
