cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")
project(zynaddsubfx)
set(VERSION "2.4.4")

enable_testing()
include(CTestConfig.cmake)
#Currently the only directory that uses cmake
add_subdirectory(src)

install(FILES AUTHORS.txt COPYING FAQ.txt HISTORY.txt README.txt
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
