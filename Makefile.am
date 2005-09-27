AUTOMAKE_OPTIONS = foreign 1.4

dist-hook:
	rm -rf `find $(distdir) -name \*.moc`


install-exec-hook:
	cd $(DESTDIR)$(bindir) ; \
	strip lmms


if HAVE_RPM
$(PACKAGE)-$(VERSION).tar.gz: dist

# Rule to build RPM distribution package
rpm: $(PACKAGE)-$(VERSION).tar.gz $(PACKAGE).spec
	cp $(PACKAGE)-$(VERSION).tar.gz @RPMSOURCEDIR@
	rpmbuild -ba $(PACKAGE).spec
endif


noinst_PROGRAMS= bin2res
bin2res_SOURCES = buildtools/bin2res.cpp


INCLUDES = -I$(srcdir)/include -I.
bin_PROGRAMS = lmms



AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS)


%.moc: $(srcdir)/include/%.h
	$(MOC) -o $@ $<
	
%.ts: 
	$(LUPDATE) $(lmms_SOURCES) -ts locale/$@
	
%.qm: %.ts
	$(LRELEASE) $<



lmms_MOC =						\
		./about_dialog.moc			\
		./arp_and_chords_tab_widget.moc		\
		./audio_file_processor.moc		\
		./bb_editor.moc				\
		./bb_track.moc				\
		./browser.moc				\
		./channel_track.moc			\
		./config_mgr.moc			\
		./crystal_button.moc			\
		./envelope_and_lfo_widget.moc		\
		./envelope_tab_widget.moc		\
		./export_project_dialog.moc		\
		./kmultitabbar.moc			\
		./group_box.moc				\
		./knob.moc				\
		./lcd_spinbox.moc			\
		./lmms_main_win.moc			\
		./midi_out.moc				\
		./mixer.moc				\
		./name_label.moc			\
		./nstate_button.moc			\
		./pattern.moc				\
		./piano_roll.moc			\
		./piano_widget.moc			\
		./pixmap_button.moc			\
		./plucked_string_synth.moc		\
		./project_notes.moc			\
		./rename_dialog.moc			\
		./sample_buffer.moc			\
		./sample_track.moc			\
		./setup_dialog.moc			\
		./side_bar.moc				\
		./side_bar_widget.moc			\
		./song_editor.moc			\
		./sound_generator.moc			\
		./surround_area.moc			\
		./tab_bar.moc				\
		./tab_button.moc			\
		./tab_widget.moc				\
		./timeline.moc				\
		./track_container.moc			\
		./track.moc				\
		./triple_oscillator.moc			\
		./visualization_widget.moc


BUILT_SOURCES = $(lmms_MOC)

#lmms_TRANSLATIONS = $(wildcard $(srcdir)/locale/*.qm $(srcdir)/locale/*.ts)

lmms_EMBEDDED_RESOURCES = $(wildcard $(srcdir)/resources/*png AUTHORS COPYING)

./embedded_resources.h: $(lmms_EMBEDDED_RESOURCES) bin2res
	$(top_builddir)/bin2res $(lmms_EMBEDDED_RESOURCES) > $@

./embed.o: ./embedded_resources.h


lmms_SOURCES =								\
		$(srcdir)/src/audio/audio_alsa.cpp			\
		$(srcdir)/src/audio/audio_device.cpp			\
		$(srcdir)/src/audio/audio_file_device.cpp		\
		$(srcdir)/src/audio/audio_file_ogg.cpp			\
		$(srcdir)/src/audio/audio_file_wave.cpp			\
		$(srcdir)/src/audio/audio_jack.cpp			\
		$(srcdir)/src/audio/audio_oss.cpp			\
		$(srcdir)/src/audio/audio_sample_recorder.cpp		\
		$(srcdir)/src/audio/audio_sdl.cpp			\
		$(srcdir)/src/core/about_dialog.cpp			\
		$(srcdir)/src/core/arp_and_chords_tab_widget.cpp	\
		$(srcdir)/src/core/bb_editor.cpp			\
		$(srcdir)/src/core/browser.cpp				\
		$(srcdir)/src/core/config_mgr.cpp			\
		$(srcdir)/src/core/envelope_and_lfo_widget.cpp		\
		$(srcdir)/src/core/envelope_tab_widget.cpp		\
		$(srcdir)/src/core/export_project_dialog.cpp		\
		$(srcdir)/src/core/lmms_main_win.cpp			\
		$(srcdir)/src/core/main.cpp				\
		$(srcdir)/src/core/mixer.cpp				\
		$(srcdir)/src/core/name_label.cpp			\
		$(srcdir)/src/core/note.cpp				\
		$(srcdir)/src/core/note_play_handle.cpp			\
		$(srcdir)/src/core/piano_roll.cpp			\
		$(srcdir)/src/core/piano_widget.cpp			\
		$(srcdir)/src/core/plugin_management.cpp		\
		$(srcdir)/src/core/preset_preview_play_handle.cpp	\
		$(srcdir)/src/core/sample_play_handle.cpp		\
		$(srcdir)/src/core/setup_dialog.cpp			\
		$(srcdir)/src/core/song_editor.cpp			\
		$(srcdir)/src/core/sound_generator.cpp			\
		$(srcdir)/src/core/track.cpp				\
		$(srcdir)/src/core/track_container.cpp			\
		$(srcdir)/src/core/surround_area.cpp			\
		$(srcdir)/src/core/timeline.cpp				\
		$(srcdir)/src/lib/buffer_allocator.cpp			\
		$(srcdir)/src/lib/clipboard.cpp				\
		$(srcdir)/src/lib/embed.cpp				\
		$(srcdir)/src/lib/mmp.cpp				\
		$(srcdir)/src/lib/oscillator.cpp			\
		$(srcdir)/src/lib/sample_buffer.cpp			\
		$(srcdir)/src/midi/midi_alsa_raw.cpp			\
		$(srcdir)/src/midi/midi_device.cpp			\
		$(srcdir)/src/midi/midi_file.cpp			\
		$(srcdir)/src/midi/midi_mapper.cpp			\
		$(srcdir)/src/midi/midi_oss.cpp				\
		$(srcdir)/src/soundgenerators/audio_file_processor.cpp	\
		$(srcdir)/src/soundgenerators/midi_out.cpp		\
		$(srcdir)/src/soundgenerators/plucked_string_synth.cpp	\
		$(srcdir)/src/soundgenerators/triple_oscillator.cpp	\
		$(srcdir)/src/tracks/bb_track.cpp			\
		$(srcdir)/src/tracks/channel_track.cpp			\
		$(srcdir)/src/tracks/pattern.cpp			\
		$(srcdir)/src/tracks/sample_track.cpp			\
		$(srcdir)/src/widgets/crystal_button.cpp		\
		$(srcdir)/src/widgets/group_box.cpp			\
		$(srcdir)/src/widgets/kmultitabbar.cpp			\
		$(srcdir)/src/widgets/knob.cpp				\
		$(srcdir)/src/widgets/lcd_spinbox.cpp			\
		$(srcdir)/src/widgets/led_checkbox.cpp			\
		$(srcdir)/src/widgets/nstate_button.cpp			\
		$(srcdir)/src/widgets/pixmap_button.cpp			\
		$(srcdir)/src/widgets/project_notes.cpp			\
		$(srcdir)/src/widgets/rename_dialog.cpp			\
		$(srcdir)/src/widgets/side_bar_widget.cpp		\
		$(srcdir)/src/widgets/tab_bar.cpp			\
		$(srcdir)/src/widgets/tab_widget.cpp			\
		$(srcdir)/src/widgets/text_float.cpp			\
		$(srcdir)/src/widgets/tooltip.cpp			\
		$(srcdir)/src/widgets/visualization_widget.cpp		\
		$(srcdir)/include/pch.h					\
		$(srcdir)/include/debug.h				\
		$(srcdir)/include/lmms_main_win.h			\
		$(srcdir)/include/audio_alsa.h				\
		$(srcdir)/include/audio_device.h			\
		$(srcdir)/include/audio_dummy.h				\
		$(srcdir)/include/audio_file_device.h			\
		$(srcdir)/include/audio_file_ogg.h			\
		$(srcdir)/include/audio_file_wave.h			\
		$(srcdir)/include/audio_jack.h				\
		$(srcdir)/include/audio_oss.h				\
		$(srcdir)/include/audio_sample_recorder.h		\
		$(srcdir)/include/audio_sdl.h				\
		$(srcdir)/include/interpolation.h			\
		$(srcdir)/include/mixer.h				\
		$(srcdir)/include/pattern.h				\
		$(srcdir)/include/channel_track.h			\
		$(srcdir)/include/note.h				\
		$(srcdir)/include/volume.h				\
		$(srcdir)/include/panning.h				\
		$(srcdir)/include/song_editor.h				\
		$(srcdir)/include/sound_generator.h			\
		$(srcdir)/include/audio_file_processor.h		\
		$(srcdir)/include/midi_time.h				\
		$(srcdir)/include/bb_editor.h				\
		$(srcdir)/include/piano_widget.h			\
		$(srcdir)/include/effect_board.h			\
		$(srcdir)/include/pixmap_button.h			\
		$(srcdir)/include/triple_oscillator.h			\
		$(srcdir)/include/settings.h				\
		$(srcdir)/include/plucked_string_synth.h		\
		$(srcdir)/include/rename_dialog.h			\
		$(srcdir)/include/export_project_dialog.h		\
		$(srcdir)/include/note_play_handle.h			\
		$(srcdir)/include/piano_roll.h				\
		$(srcdir)/include/basic_filters.h			\
		$(srcdir)/include/envelope_tab_widget.h			\
		$(srcdir)/include/envelope_and_lfo_widget.h		\
		$(srcdir)/include/about_dialog.h			\
		$(srcdir)/include/oscillator.h				\
		$(srcdir)/include/crystal_button.h			\
		$(srcdir)/include/arp_and_chords_tab_widget.h		\
		$(srcdir)/include/export.h				\
		$(srcdir)/include/group_box.h				\
		$(srcdir)/include/tab_widget.h				\
		$(srcdir)/include/knob.h				\
		$(srcdir)/include/browser.h				\
		$(srcdir)/include/templates.h				\
		$(srcdir)/include/gui_templates.h			\
		$(srcdir)/include/surround_area.h			\
		$(srcdir)/include/kmultitabbar.h			\
		$(srcdir)/include/side_bar.h				\
		$(srcdir)/include/side_bar_widget.h			\
		$(srcdir)/include/track.h				\
		$(srcdir)/include/track_container.h			\
		$(srcdir)/include/bb_track.h				\
		$(srcdir)/include/sample_track.h			\
		$(srcdir)/include/sample_buffer.h			\
		$(srcdir)/include/name_label.h				\
		$(srcdir)/include/play_handle.h				\
		$(srcdir)/include/mmp.h					\
		$(srcdir)/include/midi_device.h				\
		$(srcdir)/include/midi_file.h				\
		$(srcdir)/include/midi_alsa_raw.h			\
		$(srcdir)/include/midi_oss.h				\
		$(srcdir)/include/clipboard.h				\
		$(srcdir)/include/types.h				\
		$(srcdir)/include/qt3support.h				\
		$(srcdir)/include/embed.h				\
		$(srcdir)/include/timeline.h				\
		$(srcdir)/include/config_mgr.h				\
		$(srcdir)/include/spc_bg_hndl_widget.h			\
		$(srcdir)/include/tab_bar.h				\
		$(srcdir)/include/tab_button.h				\
		$(srcdir)/include/project_notes.h			\
		$(srcdir)/include/visualization_widget.h		\
		$(srcdir)/include/endian_handling.h			\
		$(srcdir)/include/preset_preview_play_handle.h		\
		$(srcdir)/include/sample_play_handle.h			\
		$(srcdir)/include/midi.h				\
		$(srcdir)/include/midi_out.h				\
		$(srcdir)/include/nstate_button.h			\
		$(srcdir)/include/midi_dummy.h				\
		$(srcdir)/include/midi_mapper.h				\
		$(srcdir)/include/buffer_allocator.h			\
		$(srcdir)/include/lcd_spinbox.h				\
		$(srcdir)/include/tooltip.h				\
		$(srcdir)/include/led_checkbox.h			\
		$(srcdir)/include/text_float.h				\
		$(srcdir)/include/setup_dialog.h


EXTRA_DIST =								\
		$(lmms_EMBEDDED_RESOURCES)				\
		$(srcdir)/include/sgs


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

if HAVE_LIBSRC
LIB_SRC_LDADD = -lsamplerate
endif

if HAVE_LIBSF
LIB_SF_LDADD = -lsndfile
endif

lmms_LDADD = $(QT_LDADD) $(LIB_SDL_LDADD) $(LIB_ASOUND_LDADD) $(LIB_JACK_LDADD) $(LIB_SDL_SOUND_LDADD) $(LIB_VORBIS_LDADD) $(LIB_SRC_LDADD) $(LIB_SF_LDADD)

SUBDIRS = artwork locale midi-maps presets projects samples

