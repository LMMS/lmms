INCLUDE(BuildPlugin)

BUILD_PLUGIN(bassbooster bass_booster.cpp bassbooster_controls.cpp bassbooster_control_dialog.cpp MOCFILES bassbooster_controls.h EMBEDDED_RESOURCES *.png)
