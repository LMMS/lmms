# Toolchain for cross-compiling Windows binaries from Linux using MinGW-w64 + vcpkg

set(WIN64                           TRUE)

set(CMAKE_SYSTEM_PROCESSOR          x86_64)
set(CMAKE_SYSTEM_PROCESSOR32        i686)

set(CMAKE_TOOLCHAIN_FILE_32         "${CMAKE_CURRENT_LIST_DIR}/x86-mingw-vcpkg.cmake")

include(${CMAKE_CURRENT_LIST_DIR}/common/mingw-vcpkg.cmake)
