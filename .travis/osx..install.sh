#!/usr/bin/env bash

set -e

PACKAGES="cmake pkg-config libogg libvorbis lame libsndfile libsamplerate lilv lv2 jack sdl libgig libsoundio stk fluid-synth portaudio node fltk qt carla"

if "${TRAVIS}"; then
   PACKAGES="$PACKAGES ccache"
fi

# removing already installed packages from the list
for p in $(brew list); do
	PACKAGES=${PACKAGES//$p/}
done;

# shellcheck disable=SC2086
brew install $PACKAGES

# fftw tries to install gcc which conflicts with travis
brew install fftw --ignore-dependencies

npm install -g appdmg
