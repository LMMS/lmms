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

PPA_URL=$PPA_ROOT/dists/$PPA_DISTRO/main/binary-$PPA_ARCH/Packages

ppa_dir=./ppa/

temp_file=/tmp/ppa_listing_$$

echo "Connecting to $PPA_HOST to get list of packages..."
wget -qO- $PPA_URL |grep "Filename:" > $temp_file

line_count=`wc -l $temp_file |awk '{print $1}'`

echo "Found $line_count packages for download..."

echo "Downloading packages.  They will be saved to $ppa_dir"

mkdir $ppa_dir

for j in `cat $temp_file` ; do
	if [ "$j" = "Filename:" ] ; then 
		continue 
	fi
	echo "Downloading $j..."
	wget -O $ppa_dir$(basename $j) $PPA_ROOT/$j
done


echo "Cleaning up temporary files..."
rm -rf $temp_file

echo "Packages have been saved to $ppa_dir.  Please run extract_debs.sh"




