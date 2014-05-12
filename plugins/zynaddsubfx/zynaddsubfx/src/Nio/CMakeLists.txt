#Defaults:
#         - Wave Output (enabled with the record function)
#         - Null Output
#         - Null Output Running by default
#         - Managed with OutMgr
set(zynaddsubfx_nio_SRCS
    WavEngine.cpp
    NulEngine.cpp
    AudioOut.cpp
    MidiIn.cpp
    OutMgr.cpp
    InMgr.cpp
    Engine.cpp
    EngineMgr.cpp
    Nio.cpp
    )

set(zynaddsubfx_nio_lib )

add_definitions(-DOUT_DEFAULT="${DefaultOutput}")
add_definitions(-DIN_DEFAULT="${DefaultInput}")

if(JackEnable)
    include_directories(${JACK_INCLUDE_DIR})
    list(APPEND zynaddsubfx_nio_SRCS JackEngine.cpp)
    list(APPEND zynaddsubfx_nio_lib ${JACK_LIBRARIES})
endif(JackEnable)

if(PaEnable)
    include_directories(${PORTAUDIO_INCLUDE_DIR})
    list(APPEND zynaddsubfx_nio_SRCS PaEngine.cpp)
    list(APPEND zynaddsubfx_nio_lib ${PORTAUDIO_LIBRARIES})
endif(PaEnable)

if(AlsaEnable)
    list(APPEND zynaddsubfx_nio_SRCS AlsaEngine.cpp)
    list(APPEND zynaddsubfx_nio_lib ${ASOUND_LIBRARY})
endif(AlsaEnable)

if(OssEnable)
    list(APPEND zynaddsubfx_nio_SRCS OssEngine.cpp)
endif(OssEnable)


add_library(zynaddsubfx_nio STATIC
    ${zynaddsubfx_nio_SRCS} 
    )
