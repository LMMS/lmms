#!/bin/sh

# Add WineHQ APT repo
sudo dpkg --add-architecture i386
sudo apt-get install --yes wget gpg
sudo mkdir -pm755 /etc/apt/keyrings
wget -O - https://dl.winehq.org/wine-builds/winehq.key | \
    sudo gpg --dearmor -o /etc/apt/keyrings/winehq-archive.key -
. /etc/os-release
sudo wget -NP /etc/apt/sources.list.d/ \
    "https://dl.winehq.org/wine-builds/ubuntu/dists/${UBUNTU_CODENAME}/winehq-${UBUNTU_CODENAME}.sources"
sudo apt-get update -y
