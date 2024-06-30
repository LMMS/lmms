#!/bin/sh

# Currently using these PPA packages:
#   fftw-mingw-w64
#   libgig-mingw-w64
#   qt5base-mingw-w64

echo "deb http://ppa.launchpad.net/tobydox/mingw-w64/ubuntu focal main" >> /etc/apt/sources.list
apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 72931B477E22FEFD47F8DECE02FE5F12ADDE29B2
sudo apt-get update
