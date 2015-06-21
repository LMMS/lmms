cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
project(zynaddsubfx)
set(VERSION "2.5.0")


#Include RTOSC
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rtosc/CMakeLists.txt")
    message(STATUS "RTOSC NOT FOUND")
    message(STATUS "Attempting to checkout submodule")
    find_package(Git REQUIRED)
    execute_process(COMMAND git submodule update --init)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rtosc/CMakeLists.txt")
        message(FATAL_ERROR "FAILED TO CHECKOUT RTOSC\n"
        "please check file permissions and your network")
    endif()
else()
    message(STATUS "Found Rtosc Submodule...")
endif()

set(RTOSC_NO_INSTALL TRUE)
include("rtosc/cmake/ColorMessage.cmake")
add_subdirectory(rtosc)
include_directories(rtosc/include)

enable_testing()
include(CTestConfig.cmake)
#Currently the only directory that uses cmake
add_subdirectory(src)

install(FILES AUTHORS.txt COPYING FAQ.txt HISTORY.txt README.adoc
	DESTINATION share/doc/zynaddsubfx
	)
install(FILES zynaddsubfx-jack.desktop zynaddsubfx-alsa.desktop
    DESTINATION share/applications)
install(FILES zynaddsubfx.svg
    DESTINATION share/pixmaps)
install(DIRECTORY instruments/banks
    DESTINATION share/zynaddsubfx)
install(DIRECTORY instruments/examples
    DESTINATION share/zynaddsubfx)
