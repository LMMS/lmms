#!/usr/bin/env bash

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUSE_WERROR=ON -DWANT_QT5=$QT5 ..
