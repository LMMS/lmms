INCLUDE(BuildPlugin)

SET(CMAKE_CXX_FLAGS "-fexceptions ${WERROR_FLAGS}")
BUILD_PLUGIN(xpressive expressive_plugin.cpp exprfront.cpp expressive_plugin.h MOCFILES expressive_plugin.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
