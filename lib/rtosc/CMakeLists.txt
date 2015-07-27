cmake_minimum_required(VERSION 2.8)
project(RTOSC)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)

include("cmake/ColorMessage.cmake")
SET(BUILD_RTOSC_EXAMPLES FALSE CACHE BOOL
    "Build RTOSC Example Programs")

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    include(GNUInstallDirs)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
if(NOT WIN32)
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -O2 -fPIC -std=c99   -Wall -Wextra")
    set(CPP_FLAGS       "-O2 -fPIC -std=c++11 -Wall -Wextra -Wno-unused-parameter")
endif()

if(CMAKE_COMPILER_IS_GNUCC)
    execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion
        OUTPUT_VARIABLE GCC_VERSION)
    if (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL
            4.7)
        message(STATUS "Found GCC Version >= 4.7")
    else()
        message(WARNING "GCC Version '${GCC_VERSION}' Is Quite Old, Please Consider Upgrading")
        set(CPP_FLAGS "-O2 -fPIC -std=c++0x -Wall -Wextra -Wno-unused-parameter")
    endif()
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CPP_FLAGS}")

add_library(rtosc STATIC src/rtosc.c src/dispatch.c)
add_library(rtosc-cpp STATIC src/cpp/ports.cpp src/cpp/miditable.cpp
    src/cpp/midimapper.cpp
    src/cpp/thread-link.cpp
    src/cpp/undo-history.cpp
    src/cpp/subtree-serialize.cpp)
target_link_libraries(rtosc-cpp rtosc)

# Example Code
find_package(FLTK)
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(NTK ntk)
    pkg_check_modules(JACK jack)
    pkg_check_modules(LIBLO liblo>=0.28)
endif()

if(FLTK_FOUND)
    execute_process(COMMAND fltk-config --api-version
        OUTPUT_VARIABLE FLTK_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE)
endif(FLTK_FOUND)

if(NTK_FOUND)
    #Default to NTK, as everything looks better there
    set(GUI_FOUND TRUE)
    set(GUI_LIBRARIES ${NTK_LDFLAGS})
    include_directories(${NTK_INCLUDE_DIRS})
elseif(FLTK_FOUND AND (FLTK_VERSION STREQUAL "1.3"))
    set(GUI_FOUND TRUE)
    set(GUI_LIBRARIES ${FLTK_LIBRARIES})
else()
    set(GUI_FOUND FALSE)
endif()


if(JACK_FOUND AND GUI_FOUND AND BUILD_RTOSC_EXAMPLES)
    add_executable(simple-example example/simple/ui.cpp example/simple/synth.cpp)
    target_link_libraries(simple-example m rtosc ${GUI_LIBRARIES}
        ${JACK_LDFLAGS})

    add_executable(complex-example example/complex/audio.cpp
        example/complex/synth.cpp example/complex/window.cpp
        example/complex/Fl_Osc_Dial.cpp example/complex/Fl_Osc_Slider.cpp
        example/complex/Fl_Osc_Button.cpp)
    target_link_libraries(complex-example m rtosc rtosc-cpp ${GUI_LIBRARIES}
        ${JACK_LDFLAGS})
    #TODO add complex example (ie klatter (an unplublished project for those reading))
endif()

if(BUILD_RTOSC_EXAMPLES)
    add_executable(modular-example
        example/modular/Echo.cpp
        example/modular/EffectMgr.cpp
        example/modular/LFO.cpp
        example/modular/main.cpp
        example/modular/Oscil.cpp
        example/modular/Synth.cpp
        example/modular/util.cpp
        )
    target_link_libraries(modular-example m rtosc rtosc-cpp)
endif(BUILD_RTOSC_EXAMPLES)


# Testing code
enable_testing()
macro(maketest fname)
    add_executable(${fname} test/${fname}.c)
    add_test(${fname} ${fname})
    target_link_libraries(${fname} rtosc)
endmacro(maketest)

macro(maketestcpp fname)
    add_executable(${fname} test/${fname}.cpp)
    add_test(${fname} ${fname})
    target_link_libraries(${fname} rtosc-cpp rtosc)
endmacro(maketestcpp)

maketest(osc-spec)
maketest(simple-messages)
maketest(null-messages)
maketest(bundles)
maketest(patterns)
maketest(nested-bundles)
maketest(fat-message)
maketest(empty-strings)
maketest(message-alignment)
if(LIBLO_FOUND)
    include_directories(${LIBLO_INCLUDE_DIRS})
    maketest(liblo)
    target_link_libraries(liblo ${LIBLO_LDFLAGS})
endif()

if(JACK_FOUND)
    include_directories(${JACK_INCLUDE_DIRS})
endif()

maketestcpp(metadata)
maketestcpp(headerlib)
maketestcpp(test-walker)
maketestcpp(performance)

maketestcpp(undo-test)
maketestcpp(serializer)
maketestcpp(sugar)
maketestcpp(typed-template-test)

# Documentation
find_package(Doxygen)
SET(Documentation FALSE CACHE BOOL "Enable building documentation")

if(DOXYGEN_FOUND AND Documentation)
    set(CMAKE_DOXYGEN_INPUT_LIST ${CMAKE_SOURCE_DIR}/include/)
    SET(DOXYGEN_OUTPUT_DIR html)
    CONFIGURE_FILE(Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    SET(HTML_TARGET “html” )
    ADD_CUSTOM_TARGET(${HTML_TARGET} ALL
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
endif()


#Installation
if(CMAKE_SYSTEM_NAME MATCHES "Linux" AND NOT DEFINED RTOSC_NO_INSTALL)
    if(PKG_CONFIG_FOUND)
        configure_file(librtosc.pc.cmake
            ${CMAKE_BINARY_DIR}/librtosc.pc @ONLY)
        install(FILES ${CMAKE_BINARY_DIR}/librtosc.pc
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/")
    endif()
    install(FILES include/rtosc/rtosc.h
        include/rtosc/thread-link.h
        include/rtosc/ports.h
        include/rtosc/miditable.h
        include/rtosc/port-sugar.h
        include/rtosc/undo-history.h
        include/rtosc/subtree-serialize.h
        include/rtosc/typed-message.h
        DESTINATION include/rtosc)
    install(TARGETS rtosc rtosc-cpp
        DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()


#Describe overall configuration
message(STATUS)
message(STATUS "${ColorBold}Rtosc Build Configuration")
message(STATUS             "=========================${ColorReset}")
message(STATUS)

if(PKG_CONFIG_FOUND)
    message(STATUS "PkgConfig enabled  -- ${Green}package found${ColorReset}")
else()
    message(STATUS "PkgConfig disabled -- ${Yellow}package NOT found${ColorReset}")
endif()
if(LIBLO_FOUND)
    message(STATUS "Liblo enabled      -- ${Green}package found${ColorReset}")
else()
    message(STATUS "Liblo disabled     -- ${Yellow}package NOT found${ColorReset}")
endif()
if(JACK_FOUND)
    message(STATUS "JACK enabled       -- ${Green}package found${ColorReset}")
else()
    message(STATUS "JACK disabled      -- ${Yellow}package NOT found${ColorReset}")
endif()
if(NTK_FOUND)
    message(STATUS "NTK enabled        -- ${Green}package found${ColorReset}")
else()
    message(STATUS "NTK disabled       -- ${Yellow}package NOT found${ColorReset}")
endif()

if(FLTK_FOUND)
    if(FLTK_VERSION STREQUAL "1.3")
        message(STATUS "FLTK enabled       -- ${Green}package found${ColorReset}")
    else()
        message(STATUS "FLTK disabled      -- ${Yellow}package found in unsupported version (expected 1.3)${ColorReset}")
    endif()
else()
    message(STATUS "FLTK disabled      -- ${Yellow}package NOT found${ColorReset}")
endif()
