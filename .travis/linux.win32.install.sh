#!/usr/bin/env bash

PACKAGES="nsis cloog-isl libmpc2 qt4-linguist-tools mingw32
	mingw32-x-sdl mingw32-x-libvorbis mingw32-x-fluidsynth mingw32-x-stk
	mingw32-x-glib2 mingw32-x-portaudio mingw32-x-libsndfile mingw32-x-fftw
	mingw32-x-flac mingw32-x-fltk mingw32-x-libsamplerate
	mingw32-x-pkgconfig mingw32-x-binutils mingw32-x-gcc mingw32-x-runtime
	mingw32-x-libgig mingw32-x-libsoundio"

if [ $QT5 ]; then
	PACKAGES="$PACKAGES mingw32-x-qt5base"
else
	PACKAGES="$PACKAGES mingw32-x-qt"
fi

sudo apt-get install -y $PACKAGES
