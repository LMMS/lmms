# this one is important
SET(CMAKE_SYSTEM_NAME Windows)
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

SET(CMAKE_SYSTEM_PROCESSOR i686)
SET(CMAKE_INSTALL_PREFIX /opt/mingw)

SET(CC_PREFIX /opt/mingw)

# specify the cross compiler
SET(CMAKE_C_COMPILER   ${CC_PREFIX}/bin/i586-mingw32-gcc)
SET(CMAKE_CXX_COMPILER ${CC_PREFIX}/bin/i586-mingw32-g++)

# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH  /opt/mingw)

set(QT_BINARY_DIR   ${CC_PREFIX}/bin)
set(QT_LIBRARY_DIR  ${CC_PREFIX}/lib)
set(QT_QTCORE_LIBRARY   ${CC_PREFIX}/lib/libQtCore4.a)
set(QT_INCLUDE_DIR ${CC_PREFIX}/include/qt4)
set(QT_QTCORE_INCLUDE_DIR ${CC_PREFIX}/include/qt4/QtCore)
set(QT_MKSPECS_DIR  ${CC_PREFIX}/share/qt4/mkspecs)
set(QT_MOC_EXECUTABLE  ${QT_BINARY_DIR}/moc.exe)
set(QT_RCC_EXECUTABLE  ${QT_BINARY_DIR}/rcc.exe)
set(QT_QMAKE_EXECUTABLE  /usr/bin/qmake)
set(QT_UIC_EXECUTABLE  ${QT_BINARY_DIR}/uic.exe)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

SET(PKG_CONFIG_EXECUTABLE ${CC_PREFIX}/bin/pkg-config)

