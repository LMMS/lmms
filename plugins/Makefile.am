if VST_SUPPORT
VST_DIRS=vst_base vst_effect vestige
endif

if STK_SUPPORT
STK_DIR=stk
endif

if FLUIDSYNTH_SUPPORT
SF2PLAYER_DIR=sf2_player
endif

if HAVE_LIBFFTW3
SPECTRUMANALYZER_DIR=spectrum_analyzer
endif

SUBDIRS =			\
	audio_file_processor	\
	bass_booster		\
	bit_invader		\
	flp_import		\
	kicker			\
	ladspa_browser		\
	ladspa_effect		\
	lb302 			\
	live_tool		\
	midi_import		\
	organic			\
	patman			\
	peak_controller_effect	\
	$(SF2PLAYER_DIR)	\
	$(SINGERBOT_DIR)	\
	$(SPECTRUMANALYZER_DIR)	\
	stereo_enhancer		\
	stereo_matrix		\
	$(STK_DIR)		\
	triple_oscillator	\
	vibed			\
	$(VST_DIRS)		\
	$(EXTRA_PLUGINS)
