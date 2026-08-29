IF(LMMS_HAVE_LV2)
	include_directories(SYSTEM ${LV2_INCLUDE_DIRS})
	include_directories(SYSTEM ${Lilv_INCLUDE_DIRS})
	include_directories(SYSTEM ${SUIL_INCLUDE_DIRS})
	INCLUDE(BuildPlugin)
	BUILD_PLUGIN(lv2effect Lv2Effect.cpp Lv2FxControls.cpp Lv2FxControlDialog.cpp Lv2Effect.h Lv2FxControls.h Lv2FxControlDialog.h
		MOCFILES Lv2Effect.h Lv2FxControls.h Lv2FxControlDialog.h
		EMBEDDED_RESOURCES logo.png)
ENDIF(LMMS_HAVE_LV2)
