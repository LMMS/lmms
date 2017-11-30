#!/usr/bin/env bash

PACKAGES="cmake libsndfile-dev fftw3-dev libvorbis-dev libogg-dev libmp3lame-dev
	libasound2-dev libjack-dev libsdl-dev libsamplerate0-dev libstk0-dev stk
	libfluidsynth-dev portaudio19-dev wine-dev g++-multilib libfltk1.3-dev
	libgig-dev libsoundio-dev"

# Help with unmet dependencies
PACKAGES="$PACKAGES libjack0"

if [ $QT5 ]; then
	PACKAGES="$PACKAGES qt59base qt59translations qt59tools"
else
	PACKAGES="$PACKAGES libqt4-dev"
fi

sudo apt-get install -y $PACKAGES

# Carla depends on kxstudio which creates some package conflicts (wine, etc)
# If run too early in the dependency process.  Once provided by PPA, "carla-git"
# can simply be added to $PACKAGES
sudo apt-get install -y apt-transport-https software-properties-common wget
wget https://launchpad.net/~kxstudio-debian/+archive/kxstudio/+files/kxstudio-repos_9.4.6~kxstudio1_all.deb
sudo dpkg -i kxstudio-repos_9.4.6~kxstudio1_all.deb
sudo apt-get install -y carla-git
