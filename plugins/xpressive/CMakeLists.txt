INCLUDE(BuildPlugin)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG -Dexprtk_disable_sc_andor -Dexprtk_disable_enhanced_features -Dexprtk_disable_break_continue -Dexprtk_disable_comments -Dexprtk_disable_string_capabilities -Dexprtk_disable_rtl_io_file -Dexprtk_disable_rtl_vecops ${WERROR_FLAGS}")
IF(WIN64)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj")
ENDIF()
BUILD_PLUGIN(xpressive expressive_plugin.cpp exprfront.cpp expressive_plugin.h MOCFILES expressive_plugin.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
