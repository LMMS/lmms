#!/usr/bin/env bash

PACKAGES="cmake libsndfile-dev fftw3-dev libvorbis-dev  libogg-dev
	libasound2-dev libjack-dev libsdl-dev libsamplerate0-dev libstk0-dev
	libfluidsynth-dev portaudio19-dev g++-multilib libfltk1.3-dev
	libgig-dev libsoundio-dev"

VST_PACKAGES="wine-dev libqt5x11extras5-dev qtbase5-private-dev libxcb-util0-dev libxcb-keysyms1-dev"

# Help with unmet dependencies
PACKAGES="$PACKAGES $VST_PACKAGES libjack0"

if [ "$QT5" ]; then
	PACKAGES="$PACKAGES qtbase5-dev qttools5-dev-tools qttools5-dev"
else
	PACKAGES="$PACKAGES libqt4-dev"
fi

# shellcheck disable=SC2086
sudo apt-get install -y $PACKAGES
