INCLUDE(BuildPlugin)

INCLUDE_DIRECTORIES(unrtf)

BUILD_PLUGIN(flpimport flp_import.cpp unrtf.cpp flp_import.h)
