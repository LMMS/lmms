INCLUDE(BuildPlugin)

BUILD_PLUGIN(phaser PhaserEffect.cpp PhaserControls.cpp PhaserControlDialog.cpp MOCFILES PhaserControls.h PhaserControlDialog.h ../Eq/EqFader.h EMBEDDED_RESOURCES artwork.png logo.png cutoffDotLeft.png cutoffDotRight.png)
