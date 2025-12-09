INCLUDE(BuildPlugin)

BUILD_PLUGIN(Delay2
    Delay2.cpp
    Delay2Controls.cpp
    Delay2ControlDialog.cpp
    MOCFILES Delay2Controls.h Delay2ControlDialog.h
)
