#!/usr/bin/env bash

PACKAGES="cmake libsndfile-dev fftw3-dev libvorbis-dev libogg-dev libmp3lame-dev
	libasound2-dev libjack-jackd2-dev libsdl-dev libsamplerate0-dev libstk0-dev stk
	libfluidsynth-dev portaudio19-dev g++-multilib libfltk1.3-dev fluid
	libgig-dev libsoundio-dev"

VST_PACKAGES="wine-dev libqt5x11extras5-dev qtbase5-private-dev libxcb-util0-dev libxcb-keysyms1-dev"

# Help with unmet dependencies
PACKAGES="$PACKAGES $VST_PACKAGES libjack-jackd2-0"

if [ $QT5 ]; then
	PACKAGES="$PACKAGES qttools5-dev-tools"
else
	PACKAGES="$PACKAGES libqt4-dev"
fi

sudo apt-get install -y $PACKAGES
