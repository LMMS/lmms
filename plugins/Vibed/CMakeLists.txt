INCLUDE(BuildPlugin)

BUILD_PLUGIN(vibedstrings Vibed.cpp NineButtonSelector.cpp StringContainer.cpp VibratingString.cpp Vibed.h NineButtonSelector.h StringContainer.h VibratingString.h MOCFILES Vibed.h NineButtonSelector.h EMBEDDED_RESOURCES *.png)
