# Provides a fast mechanism for filtering the plugins used at build-time
SET(PLUGIN_LIST "" CACHE STRING "List of plug-ins to build")
STRING(REPLACE " " ";" PLUGIN_LIST "${PLUGIN_LIST}")
OPTION(LMMS_MINIMAL "Build a minimal list of plug-ins" OFF)
OPTION(LIST_PLUGINS "Lists the available plugins for building" OFF)

SET(MINIMAL_LIST
	AudioFileProcessor
	Kicker
	TripleOscillator
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
	BitInvader
	Bitcrush
	CarlaBase
	CarlaPatchbay
	CarlaRack
	Compressor
	CrossoverEQ
	Delay
	Dispersion
	DualFilter
	DynamicsProcessor
	Eq
	Flanger
	GranularPitchShifter
	HydrogenImport
	LadspaBrowser
	LadspaEffect
	LOMM
	Lv2Effect
	Lv2Instrument
	Lb302
	MidiImport
	MidiExport
	MultitapEcho
	Monstro
	Nes
	OpulenZ
	Organic
	FreeBoy
	Patman
	PeakControllerEffect
	GigPlayer
	ReverbSC
	Sf2Player
	Sfxr
	Sid
	SlewDistortion
	SlicerT
	SpectrumAnalyzer
	StereoEnhancer
	StereoMatrix
	Stk
	TapTempo
	VstBase
	Vestige
	VstEffect
	Watsyn
	WaveShaper
	Vectorscope
	Vibed
	Xpressive
	ZynAddSubFx
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
