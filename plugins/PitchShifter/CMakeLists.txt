INCLUDE(BuildPlugin)

BUILD_PLUGIN(PitchShifter
    PitchShifter.cpp
    PitchShifterControls.cpp
    PitchShifterControlDialog.cpp
    MOCFILES PitchShifterControls.h PitchShifterControlDialog.h
)
