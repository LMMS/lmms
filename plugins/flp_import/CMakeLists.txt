INCLUDE(BuildPlugin)

INCLUDE_DIRECTORIES(unrtf)

BUILD_PLUGIN(flpimport flp_import.cpp flp_import.h MOCFILES EMBEDDED_RESOURCES *.png)
