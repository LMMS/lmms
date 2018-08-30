#!/usr/bin/env bash

PACKAGES="cmake pkgconfig fftw libogg libvorbis lame libsndfile libsamplerate jack sdl libgig libsoundio stk fluid-synth portaudio node fltk"

if [ $QT5 ]; then
	PACKAGES="$PACKAGES qt5"
else
	PACKAGES="$PACKAGES cartr/qt4/qt"
fi

brew install $PACKAGES ccache
# FIXME: Move "carla" to $PACKAGES once https://github.com/Homebrew/homebrew-core/pull/31560 is merged
brew install https://github.com/tresf/homebrew-core/blob/carla/Formula/carla.rb

sudo npm install -g appdmg
