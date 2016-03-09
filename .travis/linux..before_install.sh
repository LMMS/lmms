#!/usr/bin/env bash

#sudo add-apt-repository ppa:kalakris/cmake -y;
sudo add-apt-repository ppa:andrewrk/libgroove -y;
# install deps
sudo apt-get install apt-transport-https software-properties-common wget;
# Download kxstudio deb file
wget https://launchpad.net/~kxstudio-debian/+archive/kxstudio/+files/kxstudio-repos_9.2.2~kxstudio1_all.deb;
# Install it
sudo dpkg -i kxstudio-repos_9.2.2~kxstudio1_all.deb;
if [ $QT5 ]; then
	sudo add-apt-repository ppa:ubuntu-sdk-team/ppa -y
fi
sudo apt-get update -qq
