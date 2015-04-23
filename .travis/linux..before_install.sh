sudo add-apt-repository ppa:kalakris/cmake -y;
sudo apt-get update -qq

sudo apt-get install --force-yes cxxtest
wget http://downloads.sourceforge.net/liblo/liblo-0.28.tar.gz
tar xvf liblo-0.28.tar.gz && cd liblo-0.28
./configure && make && sudo make install
sudo ldconfig
cd ..
