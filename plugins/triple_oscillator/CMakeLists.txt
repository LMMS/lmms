INCLUDE(BuildPlugin)

BUILD_PLUGIN(tripleoscillator
	TripleOscillator.cpp TripleOscillator.h MOCFILES TripleOscillator.h
	QRESOURCES GLOB *.png)
