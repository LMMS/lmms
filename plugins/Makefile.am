if VST_SUPPORT
VST_DIRS=vst_base vst_effect vestige
endif

if LADSPA_SUPPORT
LADSPA_DIRS=ladspa_effect
endif

if STK_SUPPORT
STK_DIR=stk
endif

if SINGERBOT_SUPPORT
SINGERBOT_DIR=singerbot
endif

SUBDIRS =			\
	audio_file_processor	\
	bass_booster		\
	bit_invader		\
	flp_import		\
	kicker			\
	$(LADSPA_DIRS)		\
	lb302 			\
	live_tool		\
	midi_import		\
	organic			\
	patman			\
	$(SINGERBOT_DIR)	\
	$(STK_DIR)		\
	triple_oscillator	\
	$(VST_DIRS)		\
	vibed			\
	stereo_enhancer		\
	$(EXTRA_PLUGINS)
