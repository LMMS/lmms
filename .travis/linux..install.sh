PACKAGES=cmake libsndfile-dev fftw3-dev libvorbis-dev  libogg-dev              \
	libasound2-dev libjack-dev libsdl-dev libsamplerate0-dev libstk0-dev       \
	libfluidsynth-dev portaudio19-dev wine-dev g++-multilib libfltk1.3-dev     \
	libgig-dev


if [ $QT5 ]
then
	PACKAGES_QT=qtbase5-dev
else
	PACKAGES_QT=libqt4-dev
fi

sudo apt-get install -y $PACKAGES $PACKAGES_QT
