INCLUDE(BuildPlugin)

BUILD_PLUGIN(multitapecho
	MultitapEcho.cpp MultitapEchoControls.cpp MultitapEchoControlDialog.cpp
	MOCFILES MultitapEchoControls.h MultitapEchoControlDialog.h
	QRESOURCES artwork.png graph_bg.png)
