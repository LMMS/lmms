#!/usr/bin/env bash

set -e

PACKAGES="cmake libsndfile-dev fftw3-dev libvorbis-dev libogg-dev libmp3lame-dev
	libasound2-dev libjack-dev libsdl-dev libsamplerate0-dev libstk0-dev stk
	libfluidsynth-dev portaudio19-dev wine-dev g++-multilib libfltk1.3-dev
	libgig-dev libsoundio-dev"

# swh build dependencies
SWH_PACKAGES="perl libxml2-utils libxml-perl liblist-moreutils-perl"

# Help with unmet dependencies
PACKAGES="$PACKAGES $SWH_PACKAGES libjack0"

if [ "$QT5" ]; then
	PACKAGES="$PACKAGES qt58base qt58translations qt58tools"
else
	PACKAGES="$PACKAGES libqt4-dev"
fi

# shellcheck disable=SC2086
sudo apt-get install -y $PACKAGES
