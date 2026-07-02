INCLUDE(BuildPlugin)

BUILD_PLUGIN(multitapecho MultitapEcho.cpp MultitapEchoControls.cpp MultitapEchoControlDialog.cpp MOCFILES MultitapEchoControls.h MultitapEchoControlDialog.h EMBEDDED_RESOURCES artwork.png graph_bg.png)
