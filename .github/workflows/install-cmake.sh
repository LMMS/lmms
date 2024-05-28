#!/bin/sh

# Kitware APT repo for latest CMake --- See: https://apt.kitware.com/
# As of 5/21/2024, only focal and jammy are supported
if [ "$UBUNTU_VERSION" = "20.04" ] || [ "$UBUNTU_VERSION" = "22.04" ]; then
    test -f /usr/share/doc/kitware-archive-keyring/copyright || \
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null \
        | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
    echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ $(lsb_release -c -s) main" \
        | tee /etc/apt/sources.list.d/kitware.list >/dev/null \
        && apt-get update
    test -f /usr/share/doc/kitware-archive-keyring/copyright || \
        rm /usr/share/keyrings/kitware-archive-keyring.gpg
    apt-get install -y kitware-archive-keyring
fi

apt-get update && apt-get install -y cmake
