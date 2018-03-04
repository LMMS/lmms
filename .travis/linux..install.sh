#!/usr/bin/env bash

PACKAGES="cmake libsndfile-dev fftw3-dev libvorbis-dev libogg-dev libmp3lame-dev
	libasound2-dev libjack-jackd2-dev libsdl-dev libsamplerate0-dev libstk0-dev stk
	libfluidsynth-dev portaudio19-dev g++-multilib libfltk1.3-dev
	libgig-dev libsoundio-dev liblilv-dev"

VST_PACKAGES="wine-dev libqt5x11extras5-dev qtbase5-private-dev libxcb-util0-dev libxcb-keysyms1-dev"

# Help with unmet dependencies
PACKAGES="$PACKAGES $VST_PACKAGES libjack-jackd2-0"

if [ $QT5 ]; then
	PACKAGES="$PACKAGES qt59base qt59translations qt59tools"
else
	PACKAGES="$PACKAGES libqt4-dev"
fi

sudo apt-get install -y $PACKAGES

# kxstudio repo offers Carla; avoid package conflicts (wine, etc) by running last
sudo add-apt-repository -y ppa:kxstudio-debian/libs
sudo add-apt-repository -y ppa:kxstudio-debian/apps
sudo apt-get update
sudo apt-get install -y carla-git
