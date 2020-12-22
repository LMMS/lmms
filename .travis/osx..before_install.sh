#!/usr/bin/env bash

set -e

brew update
# Python 2 may cause conflicts on dependency installation
brew unlink python@2 || true
