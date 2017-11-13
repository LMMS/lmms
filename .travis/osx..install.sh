#!/usr/bin/env bash

set -e

PACKAGES="cmake pkg-config fftw libogg libvorbis lame libsndfile libsamplerate jack sdl libgig libsoundio stk portaudio node fltk"

if [ "$QT5" ]; then
	PACKAGES="$PACKAGES qt@5.5"
else
	PACKAGES="$PACKAGES cartr/qt4/qt@4"
fi

if "${TRAVIS}"; then
   PACKAGES="$PACKAGES ccache"
fi

# removing already installed packages from the list
for p in $(brew list); do
	PACKAGES=${PACKAGES//$p/}
done;

# shellcheck disable=SC2086
brew install $PACKAGES

# Recompile fluid-synth without CoreAudio per issues #649
# Ruby formula must be a URL

brew install --build-from-source "https://gist.githubusercontent.com/tresf/c9260c43270abd4ce66ff40359588435/raw/fluid-synth.rb"

sudo npm install -g appdmg
