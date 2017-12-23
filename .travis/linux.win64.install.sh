#!/usr/bin/env bash

set -e

# First, install 32-bit deps
"$TRAVIS_BUILD_DIR/.travis/linux.win32.install.sh"

MINGW_PACKAGES="mingw64-x-sdl mingw64-x-libvorbis mingw64-x-fluidsynth mingw64-x-stk
	mingw64-x-glib2 mingw64-x-portaudio mingw64-x-libsndfile
	mingw64-x-fftw mingw64-x-flac mingw64-x-fltk mingw64-x-libsamplerate
	mingw64-x-pkgconfig mingw64-x-binutils mingw64-x-gcc mingw64-x-runtime
	mingw64-x-libgig mingw64-x-libsoundio mingw64-x-lame mingw64-x-qt5base"
	
export MANUAL_PACKAGES_URLS="https://www.libsdl.org/release/SDL2-devel-2.0.7-mingw.tar.gz,install-package arch=x86_64-w64-mingw32 prefix=/opt/mingw64"

export MINGW_PACKAGES

"$TRAVIS_BUILD_DIR/.travis/linux.win.download.sh" win64

# shellcheck disable=SC2086
sudo apt-get install -y $MINGW_PACKAGES

"$TRAVIS_BUILD_DIR/.travis/linux.win.install_raw.sh"
