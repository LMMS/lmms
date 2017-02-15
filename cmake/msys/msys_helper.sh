#!/bin/bash

# Git repo information
fork="lmms"	# i.e. "lmms" or "tobydox"
branch="master"	# i.e. "master" or "stable-1.2"

# Console colors
red="\\x1B[1;31m"
green="\\x1B[1;32m"
yellow="\\x1B[1;33m"
plain="\\x1B[0m"

function info() { echo -e "\n${green}$1${plain}"; }
function warn() { echo -e "\n${yellow}$1${plain}"; }
function err() { echo -e "\n${red}$1${plain}"; exit 1;}

info "Checking for mingw environment"
env |grep MINGW
if [ $? -ne 0 ]; then
	err "  - Failed. Please relaunch using MinGW shell"
fi

info "Preparing the git directory..."
mkdir $HOME/.git; touch $HOME/.git/config > /dev/null &2>1
git config --global http.sslverify false

info "Cloning the repository..."
if [ -d ./lmms ]; then 
	warn "  - Skipping, ./lmms already exists"
else 
	git clone -b $branch https://github.com/$fork/lmms.git
fi


info "Fetching ppa using cmake/msys/fetch_ppas.sh..."
if [ -d $HOME/ppa ]; then
        warn "  - Skipping, $HOME/ppa already exists"
else
	./lmms/cmake/msys/fetch_ppa.sh	
fi


info "Extracting debs to $HOME/ppa/opt/, etc..."
if [ -d $HOME/ppa/opt ]; then
        warn "  - Skipping, $HOME/ppa/opt already exists"
else
        ./lmms/cmake/msys/extract_debs.sh
fi

info "Preparing library merge, making all qt headers writable..."
chmod u+w /mingw64/include/qt4 -R
chmod u+w /mingw32/include/qt4 -R 

info "Merging mingw headers and libraries from ppa over existing system libraries..."
find /mingw64 |grep sndfile.h

if [ $? -ne 0 ]; then 
	\cp -r $HOME/ppa/opt/mingw* /
else 
	warn "  - Skipping, sndfile.h has already been merged" 
fi

fltkver="1.3.3"
oggver="1.3.2"

info "Downloading and building fltk $fltkver"

mingw_root="/$(echo $MSYSTEM|tr '[:upper:]' '[:lower:]')"
which fluid
if [ $? -ne 0 ]; then 
	wget http://fltk.org/pub/fltk/$fltkver/fltk-$fltkver-source.tar.gz -O $HOME/fltk-source.tar.gz
	if [ $? -ne 0 ]; then
		err "ERROR: Could not download fltk.  Exiting."	
	fi
	tar zxf $HOME/fltk-source.tar.gz -C $HOME/
	pushd $HOME/fltk-$fltkver

	info "  - Compiling fltk $fltkver..."
	./configure --prefix=$mingw_root

	make

	info "  - Installing fltk..."
	make install

	if [ $? -ne 0 ]; then
        	err "ERROR: Could not build/install fltk -- Zyn needs this.  Exiting."
	fi
	
#	ln -s $mingw_root/usr/local/bin/fluid.exe $mingw_root/bin/fluid.exe 	
else
	warn "  - Skipping, fluid binary already exists" 
fi

popd

info "Downloading and building libogg $oggver"

if [ ! -e $mingw_root/lib/libogg.dll.a ]; then 
	wget http://downloads.xiph.org/releases/ogg/libogg-$oggver.tar.xz -O $HOME/libogg-source.tar.xz
	if [ $? -ne 0 ]; then
		err "ERROR: Could not download libogg.  Exiting."	
	fi
	tar xf $HOME/libogg-source.tar.xz -C $HOME/
	pushd $HOME/libogg-$oggver

	info "  - Compiling libogg $oggver..."
	./configure --prefix=$mingw_root

	make

	info "  - Installing libogg..."
	make install

	# for some reason libgig needs this
	./configure --prefix=/opt$mingw_root

	make

	info "  - Installing libogg..."
	make install

	if [ $? -ne 0 ]; then
        	err "ERROR: Could not build/install fltk -- Zyn needs this.  Exiting."
	fi
	
else
	warn "  - Skipping, libogg binary already exists" 
fi

popd

info "Cleaning up..."
rm -rf $HOME/fltk-$fltkver
info "Done."
