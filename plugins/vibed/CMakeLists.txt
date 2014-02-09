INCLUDE(BuildPlugin)

BUILD_PLUGIN(vibedstrings vibed.cpp nine_button_selector.cpp string_container.cpp vibrating_string.cpp vibed.h nine_button_selector.h string_container.h vibrating_string.h MOCFILES vibed.h nine_button_selector.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
