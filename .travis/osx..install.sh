#!/usr/bin/env bash

PACKAGES="cmake debianutils pkgconfig fftw libogg libvorbis lame libsndfile libsamplerate jack sdl libgig libsoundio stk fluid-synth portaudio node fltk carla"

if [ $QT5 ]; then
	PACKAGES="$PACKAGES qt5"
else
	PACKAGES="$PACKAGES cartr/qt4/qt"
fi

brew install $PACKAGES ccache

sudo npm install -g appdmg
