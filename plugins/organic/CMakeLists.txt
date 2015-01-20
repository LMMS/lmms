INCLUDE(BuildPlugin)

BUILD_PLUGIN(organic
	organic.cpp organic.h MOCFILES organic.h
	QRESOURCES artwork.png logo.png randomise.png randomise_pressed.png
)
