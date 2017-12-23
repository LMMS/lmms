#!/usr/bin/env bash

set -e

export MANUAL_PACKAGES_URLS="https://www.libsdl.org/release/SDL2-devel-2.0.7-mingw.tar.gz,cross"

sudo add-apt-repository ppa:tobydox/mingw-x-trusty -y
sudo apt-get update -qq
