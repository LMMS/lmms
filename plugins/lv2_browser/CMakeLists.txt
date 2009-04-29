IF(LMMS_SUPPORT_LV2)
	INCLUDE(BuildPlugin)
	BUILD_PLUGIN(lv2browser lv2_browser.cpp lv2_browser.h lv2_description.cpp lv2_description.h lv2_port_dialog.cpp lv2_port_dialog.h MOCFILES lv2_browser.h lv2_description.h lv2_port_dialog.h EMBEDDED_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.png)
ENDIF(LMMS_SUPPORT_LV2)
