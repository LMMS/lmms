cmake_minimum_required(VERSION 3.16)
project(LmmsDpiTests LANGUAGES CXX)
include(CTest)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 20)
set(DPI_QT_MAJOR 5 CACHE STRING "Qt major version for the isolated production-code tests")
get_filename_component(LMMS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
find_package(Qt${DPI_QT_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Svg Test)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/lmms_export.h" "#pragma once\n#define LMMS_EXPORT\n")
add_library(lmmsdpi STATIC
    "${LMMS_ROOT}/src/gui/DpiContext.cpp"
    "${LMMS_ROOT}/include/DpiContext.h"
    "${LMMS_ROOT}/src/gui/embed.cpp")
target_include_directories(lmmsdpi PUBLIC "${CMAKE_CURRENT_BINARY_DIR}" "${LMMS_ROOT}/include")
target_link_libraries(lmmsdpi PUBLIC Qt${DPI_QT_MAJOR}::Widgets Qt${DPI_QT_MAJOR}::Svg)
foreach(name DpiRasterTest EmbedDpiTest DpiCacheTest WindowGeometryTest)
    add_executable(${name} "${LMMS_ROOT}/tests/src/gui/${name}.cpp")
    target_link_libraries(${name} PRIVATE lmmsdpi Qt${DPI_QT_MAJOR}::Test)
    foreach(scale 1 1.25 1.5 1.5625 1.75 2)
        add_test(NAME ${name}_${scale} COMMAND ${name} -o -,txt)
        set_tests_properties(${name}_${scale} PROPERTIES ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;QT_SCALE_FACTOR=${scale};QT_AUTO_SCREEN_SCALE_FACTOR=0;QT_ENABLE_HIGHDPI_SCALING=1;QT_SCALE_FACTOR_ROUNDING_POLICY=PassThrough")
    endforeach()
endforeach()

if(WIN32)
    add_executable(DpiNativeTest "${LMMS_ROOT}/tests/src/gui/DpiNativeTest.cpp")
    target_link_libraries(DpiNativeTest PRIVATE lmmsdpi Qt${DPI_QT_MAJOR}::Test)
    # Exercise the real host manifest, not Qt's fallback awareness for manifest-less tests.
    if(MSVC)
        target_sources(DpiNativeTest PRIVATE "${LMMS_ROOT}/cmake/nsis/lmms.exe.manifest")
    else()
        enable_language(RC)
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/native-dpi.rc"
            "1 24 \"${LMMS_ROOT}/cmake/nsis/lmms.exe.manifest\"\n")
        target_sources(DpiNativeTest PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/native-dpi.rc")
    endif()
    add_test(NAME DpiNativeTest COMMAND DpiNativeTest -o -,txt)
    set_tests_properties(DpiNativeTest PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=windows;QT_SCALE_FACTOR=1;QT_SCREEN_SCALE_FACTORS=;QT_AUTO_SCREEN_SCALE_FACTOR=;QT_ENABLE_HIGHDPI_SCALING=1;QT_SCALE_FACTOR_ROUNDING_POLICY=PassThrough")
endif()
