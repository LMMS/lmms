#!/usr/bin/env bash

PACKAGES="cmake pkgconfig fftw libogg libvorbis lame libsndfile libsamplerate jack sdl libgig libsoundio stk fluid-synth portaudio node fltk"

if [ $QT5 ]; then
	PACKAGES="$PACKAGES qt5"
else
	PACKAGES="$PACKAGES cartr/qt4/qt"
fi

brew install $PACKAGES ccache
# FIXME: Move "carla" to $PACKAGES once https://github.com/Homebrew/homebrew-core/pull/31560 is merged
brew install https://gist.githubusercontent.com/tresf/a8ffb8299227c51cf11aaf3929765264/raw/e94ad17aea63084c66bcb7e90009e3d5e848a467/carla.rb

sudo npm install -g appdmg
