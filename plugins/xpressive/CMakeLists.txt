INCLUDE(BuildPlugin)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dexprtk_disable_sc_andor -Dexprtk_disable_return -Dexprtk_disable_break_continue -Dexprtk_disable_comments -Dexprtk_disable_string_capabilities -Dexprtk_disable_rtl_io_file -Dexprtk_disable_rtl_vecops ${WERROR_FLAGS}")
IF(WIN32 OR WIN64)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj -Dexprtk_disable_enhanced_features")
ENDIF()

BUILD_PLUGIN(xpressive expressive_plugin.cpp exprsynth.cpp expressive_plugin.h MOCFILES expressive_plugin.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/exprtk.patched.hpp COMMAND patch --output=exprtk.patched.hpp --input=exprtk.hpp.diff exprtk.hpp 
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
	COMMENT "patching exprtk.hpp...")
ADD_FILE_DEPENDENCIES(exprsynth.cpp ${CMAKE_CURRENT_SOURCE_DIR}/exprtk.patched.hpp)
