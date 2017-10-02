#!/bin/bash

set -eu

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
if ! env | grep MINGW; then
	err "  - Failed. Please relaunch using MinGW shell"
fi

info "Preparing the git directory..."
mkdir "$HOME/.git" || true
touch "$HOME/.git/config" > /dev/null 2>&1
git config --global http.sslverify false

info "Cloning the repository..."
if [ -d ./lmms ]; then
	warn "  - Skipping, ./lmms already exists"
else
	git clone -b $branch https://github.com/$fork/lmms.git
fi

info "Fetching ppa using cmake/msys/fetch_ppas.sh..."
if [ -d "$HOME/ppa" ]; then
        warn "  - Skipping, $HOME/ppa already exists"
else
	lmms/cmake/msys/fetch_ppa.sh
fi

info "Extracting debs to $HOME/ppa/opt/, etc..."
if [ -d "$HOME/ppa/opt" ]; then
        warn "  - Skipping, $HOME/ppa/opt already exists"
else
        lmms/cmake/msys/extract_debs.sh
fi

info "Preparing library merge, making all qt headers writable..."
chmod u+w /mingw64/include/qt4 -R
chmod u+w /mingw32/include/qt4 -R

info "Merging mingw headers and libraries from ppa over existing system libraries..."
if ! find /mingw64 | grep sndfile.h; then
	command cp -r "$HOME/ppa/opt/mingw"* /
else
	warn "  - Skipping, sndfile.h has already been merged"
fi

fltkver="1.3.3"
oggver="1.3.2"
vorbisver="1.3.5"
flacver="1.3.2"
gigver="4.0.0"
stkver="4.5.1"

mingw_root="/$(echo "$MSYSTEM"|tr '[:upper:]' '[:lower:]')"

info "Downloading and building fltk $fltkver"
if ! which fluid; then
	wget http://fltk.org/pub/fltk/$fltkver/fltk-$fltkver-source.tar.gz -O "$HOME/fltk-source.tar.gz"
	tar zxf "$HOME/fltk-source.tar.gz" -C "$HOME/"
	pushd "$HOME/fltk-$fltkver"

	info "  - Compiling fltk $fltkver..."
	./configure --prefix="$mingw_root" --enable-shared
	make

	info "  - Installing fltk..."
	make install

#	ln -s $mingw_root/usr/local/bin/fluid.exe $mingw_root/bin/fluid.exe
	popd
else
	warn "  - Skipping, fluid binary already exists"
fi

info "Downloading and building libogg $oggver"
if [ ! -e "$mingw_root/lib/libogg.dll.a" ]; then
	wget http://downloads.xiph.org/releases/ogg/libogg-$oggver.tar.xz -O "$HOME/libogg-source.tar.xz"
	tar xf "$HOME/libogg-source.tar.xz" -C "$HOME/"
	pushd "$HOME/libogg-$oggver"

	info "  - Compiling libogg $oggver..."
	./configure --prefix="$mingw_root"
	make

	info "  - Installing libogg..."
	make install
	# for some reason libgig needs this
	./configure --prefix="/opt$mingw_root"
	make

	info "  - Installing libogg..."
	make install

	popd
else
	warn "  - Skipping, libogg binary already exists"
fi

info "Downloading and building libvorbis $vorbisver"
if [ ! -e "$mingw_root/lib/libvorbis.dll.a" ]; then
	wget http://downloads.xiph.org/releases/vorbis/libvorbis-$vorbisver.tar.xz -O "$HOME/libvorbis-source.tar.xz"
	tar xf "$HOME/libvorbis-source.tar.xz" -C "$HOME/"
	pushd "$HOME/libvorbis-$vorbisver"

	info "  - Compiling libvorbis $vorbisver..."
	./configure --prefix="$mingw_root"
	make

	info "  - Installing libvorbis..."
	make install

	# for some reason libgig needs this
	./configure --prefix="/opt$mingw_root"
	make
	info "  - Installing libvorbis..."
	make install

	popd
else
	warn "  - Skipping, libvorbis binary already exists"
fi

info "Downloading and building flac $flacver"

if [ ! -e "$mingw_root/lib/libFLAC.dll.a" ]; then

	wget http://downloads.xiph.org/releases/flac/flac-$flacver.tar.xz -O "$HOME/flac-source.tar.xz"
	tar xf "$HOME/flac-source.tar.xz" -C "$HOME/"
	pushd "$HOME/flac-$flacver"

	info "  - Compiling flac $flacver..."
	./configure --prefix="$mingw_root"
	make

	info "  - Installing flac..."
	make install

	# for some reason libgig needs this
	./configure --prefix="/opt$mingw_root"
	make

	info "  - Installing flac..."
	make install

	popd
else
	warn "  - Skipping, libvorbis flac already exists"
fi

info "Downloading and building libgig $gigver"

if [ ! -e "$mingw_root/lib/libgig/libgig.dll.a" ]; then
	wget http://download.linuxsampler.org/packages/libgig-$gigver.tar.bz2 -O "$HOME/gig-source.tar.xz"
	tar xf "$HOME/gig-source.tar.xz" -C "$HOME/"
	pushd "$HOME/libgig-$gigver"

	info "  - Compiling libgig $gigver..."
	./configure --prefix="$mingw_root"
	make

	info "  - Installing libgig..."
	make install

	mv "$mingw_root/lib/bin/libakai-0.dll" "$mingw_root/bin"
	mv "$mingw_root/lib/bin/libgig-7.dll" "$mingw_root/bin"

	popd
else
	warn "  - Skipping, libgig binary already exists"
fi

info "Downloading and building stk $stkver"

if [ ! -e "$mingw_root/lib/libstk.dll" ]; then
	wget http://ccrma.stanford.edu/software/stk/release/stk-$stkver.tar.gz -O "$HOME/stk-source.tar.xz"
	tar xf "$HOME/stk-source.tar.xz" -C "$HOME/"
	pushd "$HOME/stk-$stkver"

	info "  - Compiling stk $stkver..."
	./configure --prefix="$mingw_root"
	make

	info "  - Installing stk..."
	make install

	mv "$mingw_root/lib/libstk.so" "$mingw_root/lib/libstk.dll"
	mv "$mingw_root/lib/libstk-$stkver.so" "$mingw_root/lib/libstk-$stkver.dll"

	popd
else
	warn "  - Skipping, stk binary already exists"
fi

# make a symlink to make cmake happy
if [ "$mingw_root" = "/mingw64" ]; then
	if [ ! -e /opt/mingw64/bin/x86_64-w64-mingw32-pkg-config ]; then
		ln -s /usr/bin/pkg-config /opt/mingw64/bin/x86_64-w64-mingw32-pkg-config
	fi
elif [ "$mingw_root" = "/mingw32" ]; then
	if [ ! -e /opt/mingw32/bin/i686-w64-mingw32-pkg-config ]; then
		ln -s /usr/bin/pkg-config /opt/mingw32/bin/i686-w64-mingw32-pkg-config
	fi
fi

info "Cleaning up..."
rm -rf "$HOME/fltk-$fltkver"
rm -rf "$HOME/libogg-$oggver"
rm -rf "$HOME/libvorbis-$vorbisver"
rm -rf "$HOME/flac-$flacver"
rm -rf "$HOME/libgig-$gigver"
rm -rf "$HOME/stk-$stkver"
info "Done."
