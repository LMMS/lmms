#!/usr/bin/env bash

set -e

PACKAGES="cmake libsndfile-dev fftw3-dev libvorbis-dev libogg-dev libmp3lame-dev
	libasound2-dev libjack-jackd2-dev libsdl-dev libsamplerate0-dev libstk0-dev stk
	libfluidsynth-dev portaudio19-dev g++-multilib libfltk1.3-dev fluid
	libgig-dev libsoundio-dev qt59base qt59translations qt59tools"

# swh build dependencies
SWH_PACKAGES="perl libxml2-utils libxml-perl liblist-moreutils-perl"

# VST dependencies
VST_PACKAGES="wine-dev qt59x11extras qtbase5-private-dev libxcb-util0-dev libxcb-keysyms1-dev"

# Help with unmet dependencies
PACKAGES="$PACKAGES $SWH_PACKAGES $VST_PACKAGES libjack-jackd2-0"

# shellcheck disable=SC2086
sudo apt-get install -y $PACKAGES
