#!/usr/bin/env bash

CPUS=$(cat /proc/cpuinfo | grep bogo | wc -l)

cd /lmms

mkdir build
cd build

../cmake/build_mingw$1.sh

echo Building on $CPUS CPUs
make -j$((CPUS+1))

make package

mv -v *setup.exe /lmms
