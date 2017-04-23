INCLUDE(BuildPlugin)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dexprtk_disable_sc_andor -Dexprtk_disable_return_statement -Dexprtk_disable_break_continue -Dexprtk_disable_comments -Dexprtk_disable_string_capabilities -Dexprtk_disable_rtl_io_file -Dexprtk_disable_rtl_vecops ${WERROR_FLAGS} -fexceptions")
IF(WIN32)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj -Dexprtk_disable_enhanced_features")
ENDIF()

BUILD_PLUGIN(xpressive expressive_plugin.cpp exprsynth.cpp expressive_plugin.h exprtk.patched.hpp MOCFILES expressive_plugin.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")

