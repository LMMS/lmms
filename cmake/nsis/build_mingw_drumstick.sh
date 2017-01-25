drumstick_ver=$1

cd /tmp

wget https://master.dl.sourceforge.net/project/drumstick/${drumstick_ver}/drumstick-${drumstick_ver}.tar.gz
tar xf drumstick-${drumstick_ver}.tar.gz
cd drumstick-${drumstick_ver}

# We just need library, for utils depends on Qt5Svg
# But no mingw*-x-qt5svg
sed -i "/ADD_SUBDIRECTORY(utils)/d" CMakeLists.txt
sed -i "/add_subdirectory(utils)/d" CMakeLists.txt 

mkdir -p build && cd build

case $2 in
    32)
        export MINGW=/opt/mingw32
        export PROCESSOR=i686
        ;;
    64)
        export MINGW=/opt/mingw64
        export PROCESSOR=x86_64
        ;;
    *)
        ;;
esac

export PKG_CONFIG_PATH=${MINGW}/lib/pkgconfig
export MINGW_TOOL_PREFIX=${MINGW}/bin/${PROCESSOR}-w64-mingw32-

export CMAKE_OPTS="-DUSE_WERROR=ON
                     -DLIB_SUFFIX=''
                     -DCMAKE_INSTALL_PREFIX=$MINGW
                     -DCMAKE_PREFIX_PATH=$MINGW 
                     -DMINGW_PREFIX=$MINGW 
                     -DCMAKE_SYSTEM_PROCESSOR=$PROCESSOR
                     -DCMAKE_SYSTEM_NAME=Windows 
                     -DCMAKE_SYSTEM_VERSION=1 
                     -DCMAKE_FIND_ROOT_PATH=$MINGW 
                     -DCMAKE_C_COMPILER=${MINGW_TOOL_PREFIX}gcc 
                     -DCMAKE_CXX_COMPILER=${MINGW_TOOL_PREFIX}g++ 
                     -DCMAKE_RC_COMPILER=${MINGW_TOOL_PREFIX}gcc
                     -DSTRIP=${MINGW_TOOL_PREFIX}strip
                     -DWINDRES=${MINGW_TOOL_PREFIX}windres
                     -DPKG_CONFIG_EXECUTABLE=${MINGW_TOOL_PREFIX}pkgconfig
                     -DQT_BINARY_DIR=$MINGW/bin
                     -DQT_QMAKE_EXECUTABLE=$MINGW/bin/qmake"
                     
export PATH=$PATH:$MINGW/bin

cmake ../ $CMAKE_OPTS
make && sudo make install

