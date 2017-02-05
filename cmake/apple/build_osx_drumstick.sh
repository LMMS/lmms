drumstick_ver=$1

cd /tmp

wget https://master.dl.sourceforge.net/project/drumstick/${drumstick_ver}/drumstick-${drumstick_ver}.tar.gz
tar xf drumstick-${drumstick_ver}.tar.gz
cd drumstick-${drumstick_ver}

# We just need library
sed -i "" "/ADD_SUBDIRECTORY(utils)/d" CMakeLists.txt
sed -i "" "/add_subdirectory(utils)/d" CMakeLists.txt

mkdir -p build && cd build

if [ $QT5 ];then
    STATIC_OPTS=ON
else
    STATIC_OPTS=OFF
fi

cmake ../ -DUSE_WERROR=OFF -DLIB_SUFFIX='' -DSTATIC_DRUMSTICK=$STATIC_OPTS
make && sudo make install

