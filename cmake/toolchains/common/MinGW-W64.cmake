# Toolchain for MinGW compiler

set(CMAKE_SYSTEM_NAME               Windows)
set(CMAKE_SYSTEM_VERSION            1)

set(TOOLCHAIN_PREFIX      ${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32)
set(CMAKE_C_COMPILER      ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER    ${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_RC_COMPILER     ${TOOLCHAIN_PREFIX}-windres)

set(CMAKE_FIND_ROOT_PATH  /usr/${TOOLCHAIN_PREFIX})
set(ENV{PKG_CONFIG}       /usr/bin/${TOOLCHAIN_PREFIX}-pkg-config)

if(WIN64)
	set(TOOLCHAIN_PREFIX32   ${CMAKE_SYSTEM_PROCESSOR32}-w64-mingw32)
	set(CMAKE_C_COMPILER32   ${TOOLCHAIN_PREFIX32}-gcc)
	set(CMAKE_CXX_COMPILER32 ${TOOLCHAIN_PREFIX32}-g++)
endif()

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# Search for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
