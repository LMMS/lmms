INCLUDE(BuildPlugin)

BUILD_PLUGIN(Phaser PhaserEffect.cpp PhaserControls.cpp PhaserControlDialog.cpp MOCFILES PhaserEffect.h PhaserControls.h PhaserControlDialog.h ../Eq/EqFader.h EMBEDDED_RESOURCES artwork.png logo.png cutoffDotLeft.png cutoffDotRight.png invert_active.png invert_inactive.png wet_active.png wet_inactive.png)


