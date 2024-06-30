#!/bin/sh

# TODO: Stop using this PPA once all MinGW dependencies are available through vcpkg and work with CMake.

# Currently using these PPA packages:
#   fftw-mingw-w64        (CMake cannot find vcpkg's version)
#   fltk-mingw-w64        (vcpkg fails to build it)
#   libgig-mingw-w64      (vcpkg does not support MinGW yet)
#   portaudio-mingw-w64   (CMake cannot find vcpkg's version)
#   qt5base-mingw-w64     (vcpkg takes too much memory to build)

sudo echo "deb http://ppa.launchpad.net/tobydox/mingw-w64/ubuntu focal main" >> /etc/apt/sources.list
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 72931B477E22FEFD47F8DECE02FE5F12ADDE29B2
sudo apt-get update
