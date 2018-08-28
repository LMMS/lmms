#!/usr/bin/env bash

PACKAGES="cmake pkgconfig fftw libogg libvorbis lame libsndfile libsamplerate jack sdl libgig libsoundio stk fluid-synth portaudio node fltk"

if [ $QT5 ]; then
	PACKAGES="$PACKAGES qt5"
else
	PACKAGES="$PACKAGES cartr/qt4/qt"
fi

brew install $PACKAGES ccache
brew install https://gist.github.com/tresf/a8ffb8299227c51cf11aaf3929765264/raw/c151b761ed6ad7f37f717086188c075cc1836448/carla.rb

sudo npm install -g appdmg
