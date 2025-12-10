INCLUDE(BuildPlugin)

BUILD_PLUGIN(Reverb1
    Reverb1.cpp
    Reverb1Controls.cpp
    Reverb1ControlDialog.cpp
    MOCFILES Reverb1Controls.h Reverb1ControlDialog.h
)
