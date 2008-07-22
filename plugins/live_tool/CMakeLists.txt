INCLUDE(BuildPlugin)

BUILD_PLUGIN(livetool live_tool.cpp live_tool.h EMBEDDED_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.png)
