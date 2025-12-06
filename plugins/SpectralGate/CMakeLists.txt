INCLUDE(BuildPlugin)

BUILD_PLUGIN(SpectralGate
    SpectralGate.cpp
    SpectralGateControls.cpp
    SpectralGateControlDialog.cpp
    MOCFILES
        SpectralGateControls.h
        SpectralGateControlDialog.h
)
