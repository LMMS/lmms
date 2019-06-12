#!/usr/bin/env bash

set -e

brew update

# appdmg doesn't work with old versions of Node.js
nvm install --lts
