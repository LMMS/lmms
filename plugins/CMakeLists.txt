SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Enable C++11
ADD_DEFINITIONS(-std=c++0x)

IF(LMMS_BUILD_APPLE)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
ENDIF()

INCLUDE_DIRECTORIES(
	${SAMPLERATE_INCLUDE_DIRS}
)

SET(PLUGIN_LIST "" CACHE STRING "List of plug-ins to build")

STRING(REPLACE " " ";" PLUGIN_LIST "${PLUGIN_LIST}")

OPTION(LMMS_MINIMAL "Build a minimal list of plug-ins" OFF)

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

OPTION(LMMS_EMBED_VST "Turn on to embed VSTs as subwindows. Turn off to open VSTs in separate windows." ON)
OPTION(LMMS_EMBED_VST_X11 "Turn on to embed VSTs using the X11Embed protocol. Turn off to use Qt's own embedding, which may be broken with your Qt version." ON)

IF(NOT LMMS_EMBED_VST OR NOT LMMS_BUILD_LINUX)
	SET(LMMS_EMBED_VST_X11 OFF)
ENDIF()

IF("${PLUGIN_LIST}" STREQUAL "")
	SET(PLUGIN_LIST
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
		# MidiExport - temporarily disabled, MIDI export is broken
		MultitapEcho
		monstro
		nes
		opl2
		organic
		papu
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
		vestige
		vst_base
		VstEffect
		watsyn
		waveshaper
		vibed
		zynaddsubfx
	)

ENDIF("${PLUGIN_LIST}" STREQUAL "")

FOREACH(PLUGIN ${PLUGIN_LIST})
	ADD_SUBDIRECTORY(${PLUGIN})
ENDFOREACH()
