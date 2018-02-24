#!/usr/bin/env bash

set +e

sudo add-apt-repository ppa:tobydox/mingw-x-trusty -y
sudo add-apt-repository ppa:tobydox/mingw-w64 -y


sudo apt-get update
sudo apt search "*sdl2*"
