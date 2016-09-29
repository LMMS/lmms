#!/usr/bin/env bash

PACKAGES="cmake pkgconfig fftw libogg libvorbis libsndfile libsamplerate jack sdl stk fluid-synth portaudio node fltk"

if [ $QT5 ]; then
	PACKAGES="$PACKAGES qt55"
else
	PACKAGES="$PACKAGES qt"
fi

brew install $PACKAGES

sudo npm install -g appdmg
