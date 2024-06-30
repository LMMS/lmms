#!/bin/sh

# TODO: Stop using this PPA once all MinGW dependencies are available through vcpkg and work with CMake.

# All MinGW dependencies are installed from tobydox's PPA *except* for these:
#   fluidsynth
#   libsndfile
#   lilv
#   lv2

# Some issues with vcpkg dependencies include:
#   fftw3                 (CMake cannot find vcpkg's version)
#   fltk                  (vcpkg fails to build it)
#   libgig                (vcpkg does not support MinGW yet)
#   portaudio             (CMake cannot find vcpkg's version)
#   qt5-base              (vcpkg takes too much memory to build)

sudo echo "deb http://ppa.launchpad.net/tobydox/mingw-w64/ubuntu focal main" >> /etc/apt/sources.list
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 72931B477E22FEFD47F8DECE02FE5F12ADDE29B2
sudo apt-get update
