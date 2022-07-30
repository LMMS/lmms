#!/usr/bin/env bash

set -e

sudo add-apt-repository ppa:beineri/opt-qt592-xenial -y

sudo dpkg --add-architecture i386
sudo apt-get update -qq || true
