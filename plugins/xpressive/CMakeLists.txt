INCLUDE(BuildPlugin)

SET(CMAKE_CXX_FLAGS "-Dexprtk_disable_break_continue -Dexprtk_disable_comments -Dexprtk_disable_string_capabilities -Dexprtk_disable_rtl_io_file -Dexprtk_disable_rtl_vecops ${WERROR_FLAGS}")
BUILD_PLUGIN(xpressive expressive_plugin.cpp exprfront.cpp expressive_plugin.h MOCFILES expressive_plugin.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
