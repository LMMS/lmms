sudo add-apt-repository ppa:kalakris/cmake -y;
sudo apt-get update -qq

     sudo apt-get --purge remove gcc
     sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
     sudo apt-add-repository -y ppa:dhart/ppa
     sudo apt-get -qq update
     sudo apt-get -qq install g++-4.8
     sudo apt-get -qq install gcc-4.8
     sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.8 90
     sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 90
     sudo apt-get install zlib1g-dev libmxml-dev libfftw3-dev dssi-dev libfltk1.3-dev
     sudo apt-get install --force-yes cxxtest
     wget http://downloads.sourceforge.net/liblo/liblo-0.28.tar.gz
     tar xvf liblo-0.28.tar.gz && cd liblo-0.28
     ./configure && make && sudo make install
     sudo ldconfig
     cd ..

wget http://downloads.sourceforge.net/liblo/liblo-0.28.tar.gz
tar xvf liblo-0.28.tar.gz && cd liblo-0.28
./configure && make && sudo make install
sudo ldconfig
cd ..
