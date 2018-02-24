#!/usr/bin/env bash

set +e

sudo add-apt-repository ppa:tobydox/mingw-x-trusty -y
sudo add-apt-repository "deb http://ppa.launchpad.net/tobydox/mingw-w64/ubuntu xenial main"  ppa:tobydox/mingw-w64 -y


sudo apt-get update
sudo apt-cache search sdl
