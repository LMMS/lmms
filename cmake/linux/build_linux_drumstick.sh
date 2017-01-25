drumstick_ver=$1

cd /tmp

wget https://master.dl.sourceforge.net/project/drumstick/${drumstick_ver}/drumstick-${drumstick_ver}.tar.gz
tar xf drumstick-${drumstick_ver}.tar.gz
cd drumstick-${drumstick_ver}

# We just need library
sed -i "/ADD_SUBDIRECTORY(utils)/d" CMakeLists.txt
sed -i "/add_subdirectory(utils)/d" CMakeLists.txt

mkdir -p build && cd build

cmake ../ -DUSE_WERROR=OFF -DLIB_SUFFIX='' -DCMAKE_INSTALL_PREFIX=/usr
make && sudo make install

