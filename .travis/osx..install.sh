#!/usr/bin/env bash

PACKAGES="cmake pkgconfig fftw libogg libvorbis libsndfile libsamplerate jack sdl stk fluid-synth portaudio node"

if [ $QT5 ]; then
	PACKAGES="$PACKAGES qt5"
else
	PACKAGES="$PACKAGES qt"
fi

brew reinstall $PACKAGES

sudo npm install -g appdmg

# Workaround per Homebrew bug #44806
brew reinstall fltk
if [ $? -ne 0 ]; then
  echo "Warning: fltk installation failed, trying workaround..."
  brew reinstall --devel https://raw.githubusercontent.com/dpo/homebrew/ec46018128dde5bf466b013a6c7086d0880930a3/Library/Formula/fltk.rb
fi
