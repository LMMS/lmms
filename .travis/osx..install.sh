#!/usr/bin/env bash

set -e

PACKAGES="cmake pkg-config libogg libvorbis lame libsndfile libsamplerate jack sdl libsoundio stk portaudio node fltk qt5"

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

# Recompile fluid-synth without CoreAudio per issues #649
# Ruby formula must be a URL

brew install --build-from-source "https://gist.githubusercontent.com/tresf/c9260c43270abd4ce66ff40359588435/raw/fluid-synth.rb"

# Build libgig 4.1.0 from source to avoid 3.3.0 "ISO C++11 does not allow access declarations"
brew install --build-from-source "https://raw.githubusercontent.com/tresf/homebrew-core/gig/Formula/libgig.rb"

sudo npm install -g appdmg
