#!/bin/bash
mkdir build
   cd build
   cmake ../
   make
   sudo make install || doas make install || echo "Please install either sudo or doas to install. Without propper configuration, this message may also appear."
