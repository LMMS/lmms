if VST_SUPPORT
VST_DIRS=vst_base vst_effect vestige
endif

if LADSPA_SUPPORT
LADSPA_DIRS=ladspa_base ladspa_effect
endif

if STK_SUPPORT
STK_DIR=stk
endif

SUBDIRS =			\
	audio_file_processor	\
	bass_booster		\
	bit_invader		\
	flp_import		\
	kicker			\
	$(LADSPA_DIRS)		\
	live_tool		\
	midi_import		\
	organic			\
	plucked_string_synth	\
	$(STK_DIR)		\
	triple_oscillator	\
	$(VST_DIRS)		\
	vibed
