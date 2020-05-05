# Provides a fast mechanism for filtering the plugins used at build-time
SET(PLUGIN_LIST "" CACHE STRING "List of plug-ins to build")
STRING(REPLACE " " ";" PLUGIN_LIST "${PLUGIN_LIST}")
OPTION(LMMS_MINIMAL "Build a minimal list of plug-ins" OFF)
OPTION(LIST_PLUGINS "Lists the available plugins for building" OFF)

SET(MINIMAL_LIST
	audio_file_processor
	kicker
	triple_oscillator
)

IF(LMMS_MINIMAL)
	IF("${PLUGIN_LIST}" STREQUAL "")
		STRING(REPLACE ";" " " MINIMAL_LIST_STRING "${MINIMAL_LIST}")
		MESSAGE(
"-- Using minimal plug-ins: ${MINIMAL_LIST_STRING}\n"
"   Note: You can specify specific plug-ins using -DPLUGIN_LIST=\"foo bar\""
		)
	ENDIF()
	SET(PLUGIN_LIST ${MINIMAL_LIST} ${PLUGIN_LIST})
ENDIF()

SET(LMMS_PLUGIN_LIST
	${MINIMAL_LIST}
	Amplifier
	BassBooster
	bit_invader
	Bitcrush
	carlabase
	carlapatchbay
	carlarack
	CrossoverEQ
	Delay
	DualFilter
	dynamics_processor
	Eq
	Flanger
	HydrogenImport
	ladspa_browser
	LadspaEffect
	lb302
	MidiImport
	MidiExport
	MultitapEcho
	monstro
	nes
	OpulenZ
	organic
	FreeBoy
	patman
	peak_controller_effect
	GigPlayer
	ReverbSC
	sf2_player
	sfxr
	sid
	SpectrumAnalyzer
	stereo_enhancer
	stereo_matrix
	stk
	vst_base
	vestige
	VstEffect
	watsyn
	waveshaper
	Vectorscope
	vibed
	Xpressive
	zynaddsubfx
)

IF("${PLUGIN_LIST}" STREQUAL "")
	SET(PLUGIN_LIST ${LMMS_PLUGIN_LIST})
ENDIF()

MACRO(LIST_ALL_PLUGINS)
	MESSAGE("\n\nAll possible -DPLUGIN_LIST values")
	MESSAGE("\n   KEYWORD:")
	MESSAGE("      -DLMMS_MINIMAL=True")
	FOREACH(item IN LISTS MINIMAL_LIST)
		MESSAGE("         ${item}")
	ENDFOREACH()
	MESSAGE("\n   NAME:")
	FOREACH(item IN LISTS LMMS_PLUGIN_LIST)
		MESSAGE("      ${item}")
	ENDFOREACH()
	MESSAGE("\nNote:  This value also impacts the fetching of git submodules.\n")
	MESSAGE(FATAL_ERROR "Information was requested, aborting build!")
ENDMACRO()

IF(LIST_PLUGINS)
	UNSET(LIST_PLUGINS CACHE)
	LIST_ALL_PLUGINS()
ENDIF()

IF(MSVC)
	SET(MSVC_INCOMPATIBLE_PLUGINS
		LadspaEffect
		zynaddsubfx
	)
	message(WARNING "Compiling with MSVC. The following plugins are not available: ${MSVC_INCOMPATIBLE_PLUGINS}")
	LIST(REMOVE_ITEM PLUGIN_LIST ${MSVC_INCOMPATIBLE_PLUGINS})
ENDIF()

