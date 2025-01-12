
set(WIN64                           TRUE)

set(CMAKE_SYSTEM_PROCESSOR          x86_64)
set(CMAKE_SYSTEM_PROCESSOR32        i686)

set(CMAKE_TOOLCHAIN_FILE_32			"${CMAKE_CURRENT_LIST_DIR}/MinGW-W64-32.cmake")

include(${CMAKE_CURRENT_LIST_DIR}/common/MinGW-W64.cmake)
