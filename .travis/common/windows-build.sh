#!/usr/bin/env bash
set -e

CPUS=$(grep -c bogo /proc/cpuinfo)

cd /lmms

mkdir build
cd build

../cmake/build_mingw"$1".sh

echo Building on "$CPUS" CPUs
make -j$((CPUS+1))

make package

mv -v lmms-*.exe /lmms
