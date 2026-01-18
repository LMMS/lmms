# Toolchain for cross-compiling Windows binaries from Linux using MinGW-w64 + vcpkg

set(WIN64                    FALSE)

set(CMAKE_SYSTEM_PROCESSOR   i686)

include(${CMAKE_CURRENT_LIST_DIR}/common/MinGW-W64.cmake)
