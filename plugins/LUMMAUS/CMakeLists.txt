INCLUDE(BuildPlugin)
add_subdirectory(wavtool-yawu-lmms-git)
add_subdirectory(world4lmms-git)
if (WORLD4LMMS_INCLUDE_DIR)
    include_directories(${WORLD4LMMS_INCLUDE_DIR})
else()
    message("WORLD4LMMS not found !")
endif()
if (WAVTOOL-YAWU_INCLUDE_DIR)
    include_directories(${WAVTOOL-YAWU_INCLUDE_DIR})
else()
    message("WAVTOOL-YAWU not found !")
endif()
link_libraries(world4lmms wavtool-yawu)
BUILD_PLUGIN(LUMMAUS
        LUMMAUSInstrument.cpp
        LUMMAUSInstrument.h
        Oto.cpp
        Oto.h
        Voicebank.cpp
        Voicebank.h
        MOCFILES LUMMAUSInstrument.h
        EMBEDDED_RESOURCES artwork.png logo.png)
