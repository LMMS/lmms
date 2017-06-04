#!/usr/bin/env bash

set -e

PACKAGES="cmake libsndfile-dev fftw3-dev libvorbis-dev  libogg-dev
	libasound2-dev libjack-dev libsdl-dev libsamplerate0-dev libstk0-dev
	libfluidsynth-dev portaudio19-dev wine-dev g++-multilib libfltk1.3-dev
	libgig-dev libsoundio-dev"

# Help with unmet dependencies
PACKAGES="$PACKAGES libjack0"

if [ "$QT5" ]; then
	PACKAGES="$PACKAGES qtbase5-dev qttools5-dev-tools qttools5-dev"
else
	PACKAGES="$PACKAGES libqt4-dev"
fi

# TODO: ShellCheck is not smart enough and will raise false positives.
# ShellCheck comments are workarounds and should be dropped when ShellCheck in
# the testing environment is fixed.
# See https://github.com/LMMS/lmms/issues/3473

# shellcheck disable=SC2086
sudo apt-get install -y $PACKAGES
