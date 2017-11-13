#!/usr/bin/env bash

set -e

sudo add-apt-repository ppa:beineri/opt-qt58-trusty -y
sudo add-apt-repository ppa:andrewrk/libgroove -y
sudo sed -e "s/trusty/precise/" -i \
	/etc/apt/sources.list.d/andrewrk-libgroove-trusty.list

sudo dpkg --add-architecture i386
sudo apt-get update -qq || true
