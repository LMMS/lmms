#!/usr/bin/env bash

PACKAGES="cmake pkgconfig fftw libogg libvorbis lame libsndfile libsamplerate jack sdl libgig libsoundio stk portaudio
node fltk lv2 sord"

if [ $QT5 ]; then
	PACKAGES="$PACKAGES qt5"
else
	PACKAGES="$PACKAGES cartr/qt4/qt"
fi

brew install $PACKAGES ccache

# Recompile fluid-synth without CoreAudio per issues #649
# Changes to fluid-synth.rb must be pushed to URL prior to use
url=$(git remote get-url origin)
branch=$(git symbolic-ref --short HEAD)
brew install --build-from-source $url/raw/$branch/cmake/apple/fluid-synth.rb

sudo npm install -g appdmg
