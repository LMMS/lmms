#!/usr/bin/env bash
set -e

# First, install 32-bit deps
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
. $DIR/linux.win32.install.sh


if [ $QT5 ]; then
	MINGW_PACKAGES="mingw64-x-qt5base"
else
	MINGW_PACKAGES="mingw64-x-qt"
fi

MINGW_PACKAGES="mingw64-x-sdl mingw64-x-libvorbis mingw64-x-fluidsynth mingw64-x-stk
	mingw64-x-glib2 mingw64-x-portaudio mingw64-x-libsndfile
	mingw64-x-fftw mingw64-x-flac mingw64-x-fltk mingw64-x-libsamplerate
	mingw64-x-pkgconfig mingw64-x-binutils mingw64-x-gcc mingw64-x-runtime
	mingw64-x-libgig mingw64-x-libsoundio $MINGW_PACKAGES"

export MINGW_PACKAGES

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
. $DIR/linux.win.download.sh win64


sudo apt-get install -y $MINGW_PACKAGES
