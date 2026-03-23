cmake_minimum_required(VERSION 3.16)

project(WaveConv VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia Xml)

add_executable(WaveConv
    MACOSX_BUNDLE 
    main.cpp
    mainwindow.cpp
    mainwindow.h
    synthengine.cpp
    synthengine.h
    ModularSynth.cpp
    ModularSynth.h
    pcmeditortab.cpp
    pcmeditortab.h
    oscilloscopetab.cpp
    oscilloscopetab.h
    melodyrenderertab.cpp
    melodyrenderertab.h
    effectstab.h
    effectstab.cpp
    vocalgenerator.h
    vocalgenerator.cpp
)

target_link_libraries(WaveConv PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Xml
)

if(APPLE)
    # This sets the name that appears in the macOS Finder and Menu Bar
    set_target_properties(WaveConv PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.yourdomain.WaveConv"
        MACOSX_BUNDLE_BUNDLE_NAME "WaveConv"
    )
elseif(WIN32)
    # This prevents a console window from popping up behind your app on Windows
    set_target_properties(WaveConv PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

if(Qt6_EXECUTABLE_PROPERTIES)
    set_target_properties(WaveConv PROPERTIES
        ${Qt6_EXECUTABLE_PROPERTIES}
    )
endif()

