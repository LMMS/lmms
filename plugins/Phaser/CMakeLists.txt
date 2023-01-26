INCLUDE(BuildPlugin)

BUILD_PLUGIN(phaser PhaserEffect.cpp PhaserControls.cpp PhaserControlDialog.cpp MOCFILES PhaserEffect.h PhaserControls.h PhaserControlDialog.h ../Eq/EqFader.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
TARGET_LINK_LIBRARIES(phaser hiir)
