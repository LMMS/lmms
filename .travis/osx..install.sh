#!/usr/bin/env bash

PACKAGES="cmake pkgconfig fftw libogg libvorbis libsndfile libsamplerate jack sdl stk fluid-synth portaudio node fltk"

if [ $QT5 ]; then
	PACKAGES="$PACKAGES homebrew/versions/qt55"
else
	PACKAGES="$PACKAGES cartr/qt4/qt"
fi

brew install $PACKAGES ccache

sudo npm install -g appdmg
