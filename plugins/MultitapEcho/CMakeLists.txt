INCLUDE(BuildPlugin)

BUILD_PLUGIN(multitapecho MultitapEcho.cpp MultitapEchoControls.cpp MultitapEchoControlDialog.cpp MOCFILES MultitapEchoControls.h MultitapEchoControlDialog.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
