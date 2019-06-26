#!/bin/bash

# Trusty=14.04, Precise=12.04
PPA_DISTRO=trusty

# Architecture=i386, amd64
PPA_ARCH=amd64

# These shouldn't change
PPA_HOST=http://ppa.launchpad.net
PPA_USER=tobydox
PPA_PROJECT=mingw-x-trusty
PPA_ROOT=$PPA_HOST/$PPA_USER/$PPA_PROJECT/ubuntu

PPA_URL=$PPA_ROOT/dists/$PPA_DISTRO/main/binary-$PPA_ARCH/Packages.gz

ppa_dir=./ppa/

temp_file=/tmp/ppa_listing_$$
temp_temp_file=/tmp/ppa_listing_temp_$$

skip_files="binutils openssl flac libgig libogg libvorbis x-bootstrap zlib"
skip_files="$skip_files x-runtime gcc qt_4 qt5 x-stk pkgconfig"
skip_files="$skip_files glib2 libpng"

echo "Connecting to $PPA_URL to get list of packages..."
wget -qO- $PPA_URL | gzip -d -c | grep "Filename:" > $temp_file

for j in $skip_files ; do
	grep -v "$j" $temp_file > $temp_temp_file
	mv $temp_temp_file $temp_file
done

line_count=$(wc -l $temp_file |awk '{print $1}')

echo "Found $line_count packages for download..."

echo "Downloading packages.  They will be saved to $ppa_dir"

mkdir $ppa_dir

while read -r j
do
	echo "Downloading $j..."
	echo "$PPA_ROOT/$j"
	wget -qO "$ppa_dir$(basename "$j")" "$(echo "$PPA_ROOT/$j" | sed 's/\/Filename: /\//gi')"
done < $temp_file


echo "Cleaning up temporary files..."
rm -rf $temp_file

echo "Packages have been saved to $ppa_dir.  Please run extract_debs.sh"
