#!/usr/bin/env bash

set -e

PACKAGES="cmake pkg-config fftw libogg libvorbis libsndfile libsamplerate jack sdl stk portaudio node fltk"

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
# Changes to fluid-synth.rb must be pushed to URL prior to use
if [ "$TRAVIS_EVENT_TYPE" == "pull_request" ]; then
	slug=$TRAVIS_PULL_REQUEST_SLUG
	branch=$TRAVIS_PULL_REQUEST_BRANCH
elif "${TRAVIS}"; then
	slug=$TRAVIS_REPO_SLUG
	branch=$TRAVIS_BRANCH
else
	slug="LMMS/lmms"
	branch=$(git symbolic-ref --short HEAD)
fi

brew install --build-from-source "https://raw.githubusercontent.com/${slug}/${branch}/cmake/apple/fluid-synth.rb"

sudo npm install -g appdmg
