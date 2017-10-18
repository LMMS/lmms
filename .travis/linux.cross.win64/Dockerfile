FROM ubuntu:artful
MAINTAINER Tobias Junghans <tobydox@users.sf.net>

RUN \
    apt-get update && \
    apt-get install -y software-properties-common && \
    add-apt-repository -y "ppa:tobydox/mingw-w64" && \
    apt-get update && \
    apt-get install --no-install-recommends -y --force-yes \
        git \
        cmake \
		pkg-config \
        make \
        nsis \
        tofrodos \
		qt4-linguist-tools \
		g++-mingw-w64 \
		qt5base-mingw-w64 \
		sdl-mingw-w64 \
		libvorbis-mingw-w64 \
		fluidsynth-mingw-w64 \
		stk-mingw-w64 \
		glib2-mingw-w64 \
		portaudio-mingw-w64 \
		libsndfile-mingw-w64 \
		fftw-mingw-w64 \
		flac-mingw-w64 \
		fltk-mingw-w64 \
		libsamplerate-mingw-w64 \
		libgig-mingw-w64 \
		libsoundio-mingw-w64 \
		lame-mingw-w64 \
    && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

