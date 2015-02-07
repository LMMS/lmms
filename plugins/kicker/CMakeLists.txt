INCLUDE(BuildPlugin)

BUILD_PLUGIN(kicker
	kicker.cpp kicker.h MOCFILES kicker.h
	QRESOURCES artwork.png logo.png)
