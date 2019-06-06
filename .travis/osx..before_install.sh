#!/usr/bin/env bash

brew update

# appdmg doesn't work with old versions of Node.js
nvm install --lts

# Avoid permission issues with npm and node-gyp
# https://docs.npmjs.com/resolving-eacces-permissions-errors-when-installing-packages-globally
mkdir ~/.npm-global
npm config set prefix '~/.npm-global'
export PATH=~/.npm-global/bin:$PATH
