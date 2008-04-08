AUTOMAKE_OPTIONS = foreign 1.4
ACLOCAL_AMFLAGS = -I m4

dist-hook:
	rm -rf `find $(distdir) -name \*.moc`


#install-exec-hook:
#	cd $(DESTDIR)$(bindir) ; \
#	strip lmms


if HAVE_RPM
$(PACKAGE)-$(VERSION).tar.gz: dist

# Rule to build RPM distribution package
rpm: $(PACKAGE)-$(VERSION).tar.gz $(PACKAGE).spec
	cp $(PACKAGE)-$(VERSION).tar.gz @RPMSOURCEDIR@
	rpmbuild -ba $(PACKAGE).spec
endif



SUBDIRS = buildtools data plugins


INCLUDES = -I$(srcdir)/include -I.
#-I/usr/include/wine -I/usr/include/wine/windows
bin_PROGRAMS = lmms


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS)


%.moc: $(srcdir)/include/%.h
	$(MOC) -o $@ $<

%.ts: 
	$(LUPDATE) $(lmms_SOURCES) `find plugins/ -type f -name "*.cpp"` -ts data/locale/$@

%.qm: %.ts
	$(LRELEASE) data/locale/$<


man1_MANS = lmms.1


lmms_MOC =						\
		./about_dialog.moc			\
		./automatable_button.moc		\
		./automatable_slider.moc		\
		./automation_editor.moc			\
		./automation_pattern.moc		\
		./automation_track.moc			\
		./bb_editor.moc				\
		./bb_track_container.moc		\
		./bb_track.moc				\
		./caption_menu.moc			\
		./instrument_track.moc			\
		./combobox.moc				\
		./config_mgr.moc			\
		./controller.moc			\
		./controller_connection_dialog.moc		\
		./controller_dialog.moc			\
		./controller_rack_view.moc		\
		./controller_view.moc			\
		./cpuload_widget.moc			\
		./effect_control_dialog.moc		\
		./effect_label.moc			\
		./effect_select_dialog.moc		\
		./envelope_and_lfo_parameters.moc	\
		./envelope_and_lfo_view.moc		\
		./export_project_dialog.moc		\
		./fade_button.moc			\
		./fader.moc				\
		./file_browser.moc			\
		./fx_mixer.moc				\
		./graph.moc				\
		./group_box.moc				\
		./instrument_functions.moc		\
		./instrument_function_views.moc		\
		./instrument_midi_io.moc		\
		./instrument_midi_io_view.moc		\
		./instrument_sound_shaping.moc		\
		./instrument_sound_shaping_view.moc	\
		./kmultitabbar.moc			\
		./knob.moc				\
 		./ladspa_control.moc			\
 		./ladspa_control_view.moc		\
		./lcd_spinbox.moc			\
		./led_checkbox.moc			\
		./lfo_controller.moc			\
		./main_window.moc			\
		./mixer.moc				\
		./mv_base.moc				\
		./name_label.moc			\
		./nstate_button.moc			\
		./meter_dialog.moc			\
		./midi_alsa_seq.moc			\
		./instrument_midi_io.moc		\
		./pattern.moc				\
		./piano_roll.moc			\
		./piano.moc				\
		./pixmap_button.moc			\
		./plugin_browser.moc			\
		./project_notes.moc			\
		./effect_view.moc			\
		./effect_rack_view.moc			\
		./rubberband.moc			\
		./rename_dialog.moc			\
		./sample_buffer.moc			\
		./sample_track.moc			\
		./setup_dialog.moc			\
		./side_bar.moc				\
		./side_bar_widget.moc			\
		./song.moc				\
		./song_editor.moc			\
		./surround_area.moc			\
		./tab_bar.moc				\
		./tab_button.moc			\
		./tab_widget.moc			\
		./tempo_sync_knob.moc			\
		./timeline.moc				\
		./tool_button.moc			\
		./track_container.moc			\
		./track_container_view.moc		\
		./track.moc				\
		./visualization_widget.moc		\
		./volume_knob.moc


BUILT_SOURCES = $(lmms_MOC)

lmms_EMBEDDED_RESOURCES = $(srcdir)/AUTHORS $(srcdir)/COPYING

./embedded_resources.h: $(lmms_EMBEDDED_RESOURCES)
	$(BIN2RES) $(lmms_EMBEDDED_RESOURCES) > $@

./embed.o: ./embedded_resources.h


if USE_3RDPARTY_LIBSRC
LIBSAMPLERATE_SOURCES=	$(srcdir)/src/3rdparty/samplerate/samplerate.c		\
			$(srcdir)/src/3rdparty/samplerate/samplerate.h		\
			$(srcdir)/src/3rdparty/samplerate/src_sinc.c		\
			$(srcdir)/src/3rdparty/samplerate/high_qual_coeffs.h	\
			$(srcdir)/src/3rdparty/samplerate/mid_qual_coeffs.h	\
			$(srcdir)/src/3rdparty/samplerate/fastest_coeffs.h	\
			$(srcdir)/src/3rdparty/samplerate/common.h		\
			$(srcdir)/src/3rdparty/samplerate/float_cast.h		\
			$(srcdir)/src/3rdparty/samplerate/src_zoh.c		\
			$(srcdir)/src/3rdparty/samplerate/src_linear.c
endif

THIRD_PARTY_CODE=$(LIBSAMPLERATE_SOURCES)



lmms_SOURCES =									\
		$(srcdir)/src/core/automation_pattern.cpp			\
		$(srcdir)/src/core/base64.cpp					\
		$(srcdir)/src/core/bb_track_container.cpp			\
		$(srcdir)/src/core/clipboard.cpp				\
		$(srcdir)/src/core/config_mgr.cpp				\
		$(srcdir)/src/core/controller.cpp				\
		$(srcdir)/src/core/drumsynth.cpp				\
		$(srcdir)/src/core/effect_chain.cpp				\
		$(srcdir)/src/core/effect.cpp					\
		$(srcdir)/src/core/engine.cpp					\
		$(srcdir)/src/core/envelope_and_lfo_parameters.cpp		\
		$(srcdir)/src/core/instrument_sound_shaping.cpp			\
		$(srcdir)/src/core/fx_mixer.cpp					\
		$(srcdir)/src/core/import_filter.cpp				\
		$(srcdir)/src/core/instrument.cpp				\
		$(srcdir)/src/core/instrument_functions.cpp			\
		$(srcdir)/src/core/instrument_midi_io.cpp			\
		$(srcdir)/src/core/journalling_object.cpp			\
		$(srcdir)/src/core/ladspa_2_lmms.cpp				\
		$(srcdir)/src/core/ladspa_manager.cpp				\
		$(srcdir)/src/core/ladspa_control.cpp				\
		$(srcdir)/src/core/lfo_controller.cpp				\
		$(srcdir)/src/core/main.cpp					\
		$(srcdir)/src/core/mixer.cpp					\
		$(srcdir)/src/core/mmp.cpp					\
		$(srcdir)/src/core/mv_base.cpp					\
		$(srcdir)/src/core/note.cpp					\
		$(srcdir)/src/core/note_play_handle.cpp				\
		$(srcdir)/src/core/oscillator.cpp				\
		$(srcdir)/src/core/piano.cpp					\
		$(srcdir)/src/core/plugin.cpp					\
		$(srcdir)/src/core/preset_preview_play_handle.cpp		\
		$(srcdir)/src/core/project_journal.cpp				\
		$(srcdir)/src/core/project_version.cpp				\
		$(srcdir)/src/core/sample_buffer.cpp				\
		$(srcdir)/src/core/sample_play_handle.cpp			\
		$(srcdir)/src/core/song.cpp					\
		$(srcdir)/src/core/surround_area.cpp				\
		$(srcdir)/src/core/timeline.cpp					\
		$(srcdir)/src/core/tool.cpp					\
		$(srcdir)/src/core/track.cpp					\
		$(srcdir)/src/core/track_container.cpp				\
		$(srcdir)/src/core/audio/audio_alsa.cpp				\
		$(srcdir)/src/core/audio/audio_device.cpp			\
		$(srcdir)/src/core/audio/audio_file_device.cpp			\
		$(srcdir)/src/core/audio/audio_file_ogg.cpp			\
		$(srcdir)/src/core/audio/audio_file_wave.cpp			\
		$(srcdir)/src/core/audio/audio_jack.cpp				\
		$(srcdir)/src/core/audio/audio_oss.cpp				\
		$(srcdir)/src/core/audio/audio_port.cpp				\
		$(srcdir)/src/core/audio/audio_sample_recorder.cpp		\
		$(srcdir)/src/core/audio/audio_sdl.cpp				\
		$(srcdir)/src/core/midi/midi_alsa_raw.cpp			\
		$(srcdir)/src/core/midi/midi_alsa_seq.cpp			\
		$(srcdir)/src/core/midi/midi_client.cpp				\
		$(srcdir)/src/core/midi/midi_mapper.cpp				\
		$(srcdir)/src/core/midi/midi_oss.cpp				\
		$(srcdir)/src/core/midi/midi_port.cpp				\
		$(srcdir)/src/gui/about_dialog.cpp				\
		$(srcdir)/src/gui/automation_editor.cpp				\
		$(srcdir)/src/gui/bb_editor.cpp					\
		$(srcdir)/src/gui/controller_connection_dialog.cpp \
		$(srcdir)/src/gui/effect_control_dialog.cpp			\
		$(srcdir)/src/gui/effect_select_dialog.cpp			\
		$(srcdir)/src/gui/embed.cpp					\
		$(srcdir)/src/gui/export_project_dialog.cpp			\
		$(srcdir)/src/gui/file_browser.cpp				\
		$(srcdir)/src/gui/lfo_controller_dialog.cpp			\
		$(srcdir)/src/gui/lmms_style.cpp				\
		$(srcdir)/src/gui/main_window.cpp				\
		$(srcdir)/src/gui/piano_roll.cpp				\
		$(srcdir)/src/gui/plugin_browser.cpp				\
		$(srcdir)/src/gui/setup_dialog.cpp				\
		$(srcdir)/src/gui/song_editor.cpp				\
		$(srcdir)/src/gui/string_pair_drag.cpp				\
		$(srcdir)/src/gui/track_container_view.cpp			\
		$(srcdir)/src/gui/widgets/automatable_button.cpp		\
		$(srcdir)/src/gui/widgets/automatable_slider.cpp		\
		$(srcdir)/src/gui/widgets/caption_menu.cpp			\
		$(srcdir)/src/gui/widgets/combobox.cpp				\
		$(srcdir)/src/gui/widgets/controller_rack_view.cpp		\
		$(srcdir)/src/gui/widgets/controller_view.cpp			\
		$(srcdir)/src/gui/widgets/cpuload_widget.cpp			\
		$(srcdir)/src/gui/widgets/effect_label.cpp			\
		$(srcdir)/src/gui/widgets/effect_rack_view.cpp			\
		$(srcdir)/src/gui/widgets/effect_view.cpp			\
		$(srcdir)/src/gui/widgets/fade_button.cpp			\
		$(srcdir)/src/gui/widgets/fader.cpp				\
		$(srcdir)/src/gui/widgets/graph.cpp				\
		$(srcdir)/src/gui/widgets/group_box.cpp				\
		$(srcdir)/src/gui/widgets/envelope_and_lfo_view.cpp		\
		$(srcdir)/src/gui/widgets/instrument_function_views.cpp		\
		$(srcdir)/src/gui/widgets/instrument_midi_io_view.cpp		\
		$(srcdir)/src/gui/widgets/instrument_sound_shaping_view.cpp	\
		$(srcdir)/src/gui/widgets/kmultitabbar.cpp			\
		$(srcdir)/src/gui/widgets/knob.cpp				\
		$(srcdir)/src/gui/widgets/ladspa_control_view.cpp		\
		$(srcdir)/src/gui/widgets/lcd_spinbox.cpp			\
		$(srcdir)/src/gui/widgets/led_checkbox.cpp			\
		$(srcdir)/src/gui/widgets/meter_dialog.cpp			\
		$(srcdir)/src/gui/widgets/name_label.cpp			\
		$(srcdir)/src/gui/widgets/nstate_button.cpp			\
		$(srcdir)/src/gui/widgets/pixmap_button.cpp			\
		$(srcdir)/src/gui/widgets/project_notes.cpp			\
		$(srcdir)/src/gui/widgets/rubberband.cpp			\
		$(srcdir)/src/gui/widgets/rename_dialog.cpp			\
		$(srcdir)/src/gui/widgets/side_bar_widget.cpp			\
		$(srcdir)/src/gui/widgets/tab_bar.cpp				\
		$(srcdir)/src/gui/widgets/tab_widget.cpp			\
		$(srcdir)/src/gui/widgets/text_float.cpp			\
		$(srcdir)/src/gui/widgets/tempo_sync_knob.cpp			\
		$(srcdir)/src/gui/widgets/tool_button.cpp			\
		$(srcdir)/src/gui/widgets/tooltip.cpp				\
		$(srcdir)/src/gui/widgets/visualization_widget.cpp		\
		$(srcdir)/src/gui/widgets/volume_knob.cpp			\
		$(srcdir)/src/tracks/automation_track.cpp			\
		$(srcdir)/src/tracks/bb_track.cpp				\
		$(srcdir)/src/tracks/instrument_track.cpp			\
		$(srcdir)/src/tracks/pattern.cpp				\
		$(srcdir)/include/aeffectx.h					\
		$(srcdir)/include/debug.h					\
		$(srcdir)/include/detuning_helper.h				\
		$(srcdir)/include/drumsynth.h					\
		$(srcdir)/include/main_window.h					\
		$(srcdir)/include/audio_alsa.h					\
		$(srcdir)/include/audio_device.h				\
		$(srcdir)/include/audio_dummy.h					\
		$(srcdir)/include/audio_file_device.h				\
		$(srcdir)/include/audio_file_ogg.h				\
		$(srcdir)/include/audio_file_wave.h				\
		$(srcdir)/include/audio_jack.h					\
		$(srcdir)/include/audio_oss.h					\
		$(srcdir)/include/audio_sample_recorder.h			\
		$(srcdir)/include/audio_sdl.h					\
		$(srcdir)/include/automation_editor.h				\
		$(srcdir)/include/automation_pattern.h				\
		$(srcdir)/include/automation_track.h				\
		$(srcdir)/include/interpolation.h				\
		$(srcdir)/include/lmms_constants.h				\
		$(srcdir)/include/lmms_math.h					\
		$(srcdir)/include/lmms_style.h					\
		$(srcdir)/include/mixer.h					\
		$(srcdir)/include/pattern.h					\
		$(srcdir)/include/instrument_track.h				\
		$(srcdir)/include/note.h					\
		$(srcdir)/include/volume.h					\
		$(srcdir)/include/panning.h					\
		$(srcdir)/include/song.h					\
		$(srcdir)/include/song_editor.h					\
		$(srcdir)/include/plugin.h					\
		$(srcdir)/include/plugin_view.h					\
		$(srcdir)/include/instrument.h					\
		$(srcdir)/include/instrument_view.h				\
		$(srcdir)/include/bb_editor.h					\
		$(srcdir)/include/bb_track_container.h				\
		$(srcdir)/include/piano.h					\
		$(srcdir)/include/fx_mixer.h					\
		$(srcdir)/include/pixmap_button.h				\
		$(srcdir)/include/rename_dialog.h				\
		$(srcdir)/include/export_project_dialog.h			\
		$(srcdir)/include/note_play_handle.h				\
		$(srcdir)/include/piano_roll.h					\
		$(srcdir)/include/basic_filters.h				\
		$(srcdir)/include/envelope_and_lfo_parameters.h			\
		$(srcdir)/include/envelope_and_lfo_view.h			\
		$(srcdir)/include/instrument_sound_shaping.h			\
		$(srcdir)/include/envelope_and_lfo_parameters.h			\
		$(srcdir)/include/about_dialog.h				\
		$(srcdir)/include/oscillator.h					\
		$(srcdir)/include/instrument_functions.h			\
		$(srcdir)/include/instrument_function_views.h			\
		$(srcdir)/include/instrument_sound_shaping_view.h		\
		$(srcdir)/include/export.h					\
		$(srcdir)/include/fader.h					\
		$(srcdir)/include/mv_base.h					\
		$(srcdir)/include/automatable_model.h				\
		$(srcdir)/include/automatable_model_templates.h			\
		$(srcdir)/include/graph.h					\
		$(srcdir)/include/group_box.h					\
		$(srcdir)/include/tab_widget.h					\
		$(srcdir)/include/knob.h					\
		$(srcdir)/include/file_browser.h				\
		$(srcdir)/include/plugin_browser.h				\
		$(srcdir)/include/templates.h					\
		$(srcdir)/include/gui_templates.h				\
		$(srcdir)/include/surround_area.h				\
		$(srcdir)/include/kmultitabbar.h				\
		$(srcdir)/include/side_bar.h					\
		$(srcdir)/include/side_bar_widget.h				\
		$(srcdir)/include/track.h					\
		$(srcdir)/include/track_container.h				\
		$(srcdir)/include/track_container_view.h			\
		$(srcdir)/include/bb_track.h					\
		$(srcdir)/include/caption_menu.h				\
		$(srcdir)/include/sample_track.h				\
		$(srcdir)/include/sample_buffer.h				\
		$(srcdir)/include/name_label.h					\
		$(srcdir)/include/play_handle.h					\
		$(srcdir)/include/mmp.h						\
		$(srcdir)/include/midi.h					\
		$(srcdir)/include/midi_alsa_raw.h				\
		$(srcdir)/include/midi_client.h					\
		$(srcdir)/include/midi_event_processor.h			\
		$(srcdir)/include/midi_oss.h					\
		$(srcdir)/include/midi_port.h					\
		$(srcdir)/include/midi_time.h					\
		$(srcdir)/include/clipboard.h					\
		$(srcdir)/include/types.h					\
		$(srcdir)/include/update_event.h				\
		$(srcdir)/include/embed.h					\
		$(srcdir)/include/timeline.h					\
		$(srcdir)/include/config_mgr.h					\
		$(srcdir)/include/tab_bar.h					\
		$(srcdir)/include/tab_button.h					\
		$(srcdir)/include/project_notes.h				\
		$(srcdir)/include/project_version.h				\
		$(srcdir)/include/visualization_widget.h			\
		$(srcdir)/include/endian_handling.h				\
		$(srcdir)/include/preset_preview_play_handle.h			\
		$(srcdir)/include/sample_play_handle.h				\
		$(srcdir)/include/nstate_button.h				\
		$(srcdir)/include/midi_dummy.h					\
		$(srcdir)/include/midi_mapper.h					\
		$(srcdir)/include/lcd_spinbox.h					\
		$(srcdir)/include/tooltip.h					\
		$(srcdir)/include/automatable_button.h				\
		$(srcdir)/include/automatable_slider.h				\
		$(srcdir)/include/led_checkbox.h				\
		$(srcdir)/include/text_float.h					\
		$(srcdir)/include/tempo_sync_knob.h				\
		$(srcdir)/include/volume_knob.h					\
		$(srcdir)/include/setup_dialog.h				\
		$(srcdir)/include/dummy_plugin.h				\
		$(srcdir)/include/dummy_instrument.h				\
		$(srcdir)/include/instrument_play_handle.h			\
		$(srcdir)/include/string_pair_drag.h				\
		$(srcdir)/include/instrument_midi_io.h				\
		$(srcdir)/include/instrument_midi_io_view.h			\
		$(srcdir)/include/audio_port.h					\
		$(srcdir)/include/tool.h					\
		$(srcdir)/include/tool_button.h					\
		$(srcdir)/include/cpuload_widget.h				\
		$(srcdir)/include/custom_events.h				\
		$(srcdir)/include/midi_alsa_seq.h				\
		$(srcdir)/include/micro_timer.h					\
		$(srcdir)/include/fade_button.h					\
		$(srcdir)/include/fifo_buffer.h					\
		$(srcdir)/include/combobox.h					\
		$(srcdir)/include/rubberband.h					\
		$(srcdir)/include/base64.h					\
		$(srcdir)/include/journalling_object.h				\
		$(srcdir)/include/level_object.h				\
		$(srcdir)/include/project_journal.h				\
		$(srcdir)/include/shared_object.h				\
		$(srcdir)/include/import_filter.h				\
		$(srcdir)/include/engine.h					\
		$(srcdir)/include/effect.h					\
		$(srcdir)/include/effect_chain.h				\
		$(srcdir)/include/effect_controls.h				\
		$(srcdir)/include/effect_control_dialog.h			\
		$(srcdir)/include/effect_label.h				\
		$(srcdir)/include/effect_rack_view.h				\
		$(srcdir)/include/effect_select_dialog.h			\
		$(srcdir)/include/effect_view.h					\
		$(srcdir)/include/dummy_effect.h				\
		$(srcdir)/include/ladspa-1.1.h					\
		$(srcdir)/include/sweep_oscillator.h				\
		$(srcdir)/include/meter_dialog.h				\
		$(srcdir)/include/effect_lib.h					\
		$(srcdir)/include/ladspa_manager.h				\
		$(srcdir)/include/ladspa_2_lmms.h				\
		$(srcdir)/include/ladspa_control.h				\
		$(srcdir)/include/ladspa_control_view.h				\
		$(srcdir)/include/ladspa_base.h					\
		$(srcdir)/include/controller.h					\
		$(srcdir)/include/controller_rack_view.h			\
		$(srcdir)/include/controller_view.h				\
		$(srcdir)/include/controller_dialog.h				\
		$(srcdir)/include/controller_connection_dialog.h		\
		$(srcdir)/include/lfo_controller.h				\
		$(THIRD_PARTY_CODE)

#		$(srcdir)/src/tracks/sample_track.cpp

EXTRA_DIST = $(lmms_EMBEDDED_RESOURCES) lmms.1


CLEANFILES = $(lmms_MOC) ./embedded_resources.h

if HAVE_LIBSDL
LIB_SDL_LDADD = -lSDL
endif

if HAVE_LIBSDL_SOUND
LIB_SDL_SOUND_LDADD = -lSDL_sound
endif

if HAVE_LIBASOUND
LIB_ASOUND_LDADD = -lasound
endif

if HAVE_LIBJACK
LIB_JACK_LDADD = -ljack
endif

if HAVE_LIBVORBIS
LIB_VORBIS_LDADD = -lvorbis -lvorbisenc -lvorbisfile
endif

if USE_3RDPARTY_LIBSRC
else
LIB_SRC_LDADD = -lsamplerate
endif

if HAVE_LIBSF
LIB_SF_LDADD = -lsndfile
endif

lmms_LDADD = $(QT_LDADD) $(LIB_SDL_LDADD) $(LIB_ASOUND_LDADD) $(LIB_JACK_LDADD) $(LIB_SDL_SOUND_LDADD) $(LIB_VORBIS_LDADD) $(LIB_SRC_LDADD) $(LIB_SF_LDADD)
if BUILD_LINUX
lmms_LDFLAGS = -rdynamic
endif

single-binary:
	$(CXX) $(CXXFLAGS) -DHAVE_CONFIG_H -combine -pipe -O2 -g $(INCLUDES) $(AM_CXXFLAGS) $(lmms_LDADD) $(lmms_LDFLAGS) $(lmms_SOURCES) -o lmms-single

#-rpath $(pkglibdir)

