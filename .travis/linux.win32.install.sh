#!/usr/bin/env bash

set -e

MINGW_PACKAGES="mingw32-x-sdl mingw32-x-libvorbis mingw32-x-fluidsynth mingw32-x-stk
	mingw32-x-glib2 mingw32-x-portaudio mingw32-x-libsndfile mingw32-x-fftw
	mingw32-x-flac mingw32-x-fltk mingw32-x-libsamplerate
	mingw32-x-pkgconfig mingw32-x-binutils mingw32-x-gcc mingw32-x-runtime
	mingw32-x-libgig mingw32-x-libsoundio mingw32-x-lame mingw32-x-qt5base"

# swh build dependencies
SWH_PACKAGES="perl libxml2-utils libxml-perl liblist-moreutils-perl"

export MANUAL_PACKAGES_URLS="https://www.libsdl.org/release/SDL2-devel-2.0.7-mingw.tar.gz,install-package arch=i686-w64-mingw32 prefix=/opt/mingw32"

export MINGW_PACKAGES

"$TRAVIS_BUILD_DIR/.travis/linux.win.download.sh" win32

PACKAGES="nsis cloog-isl libmpc3 qt4-linguist-tools mingw32 $MINGW_PACKAGES $SWH_PACKAGES"

# shellcheck disable=SC2086
sudo apt-get install -y $PACKAGES

"$TRAVIS_BUILD_DIR/.travis/linux.win.install_raw.sh"

# ccache 3.2 is needed because mingw32-x-gcc is version 4.9, which causes cmake
# to use @file command line passing, which in turn ccache 3.1.9 doesn't support
pushd /tmp
wget http://archive.ubuntu.com/ubuntu/pool/main/c/ccache/ccache_3.2.4-1_amd64.deb
sha256sum -c "$TRAVIS_BUILD_DIR/.travis/ccache.sha256"
sudo dpkg -i ccache_3.2.4-1_amd64.deb
popd
