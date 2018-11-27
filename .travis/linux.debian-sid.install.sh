#!/bin/sh
sudo apt-get install -y \
	debian-archive-keyring \
	dpkg \
	pbuilder

# work around a pbuilder bug which breaks ccache
# https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=666525
cd /tmp
wget http://archive.ubuntu.com/ubuntu/pool/main/p/pbuilder/pbuilder_0.229.1_all.deb
wget http://archive.ubuntu.com/ubuntu/pool/main/d/debootstrap/debootstrap_1.0.95_all.deb
sha256sum -c "$TRAVIS_BUILD_DIR/.travis/debian_pkgs.sha256"
sudo dpkg -i pbuilder_0.229.1_all.deb debootstrap_1.0.95_all.deb
cd "$OLDPWD"
