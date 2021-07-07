INCLUDE(BuildPlugin)

BUILD_PLUGIN(Phaser PhaserEffect.cpp PhaserControls.cpp PhaserControlDialog.cpp MOCFILES PhaserEffect.h PhaserControls.h PhaserControlDialog.h ../Eq/EqFader.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
