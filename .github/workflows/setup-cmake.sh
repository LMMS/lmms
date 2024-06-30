#!/bin/sh

# Add Kitware APT repo for latest CMake --- See: https://apt.kitware.com/
# As of 6/29/2024, only focal, jammy, and noble are supported
. /etc/os-release
if [ "$UBUNTU_CODENAME" = "focal" ] || [ "$UBUNTU_CODENAME" = "jammy" ] || [ "$UBUNTU_CODENAME" = "noble" ]; then
    sudo apt-get install -y wget gpg
    test -f /usr/share/doc/kitware-archive-keyring/copyright || \
        wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null \
        | gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
    echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ ${UBUNTU_CODENAME} main" \
        | sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null
    sudo apt-get update
    test -f /usr/share/doc/kitware-archive-keyring/copyright || \
        sudo rm /usr/share/keyrings/kitware-archive-keyring.gpg
    sudo apt-get install -y kitware-archive-keyring
fi
