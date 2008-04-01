AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/gui -I.


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="sf2player"


%.moc: ./%.h
	$(MOC) -o $@ $<

ui_%.h: ./%.ui
	$(UIC) -o $@ $<


MOC_FILES = ./sf2_player.moc ./patches_dialog.moc

UIC_FILES = ./ui_patches_dialog.h

BUILT_SOURCES = $(MOC_FILES) ${UIC_FILES} ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ${UIC_FILES} ./embedded_resources.h



pkglib_LTLIBRARIES = libsf2player.la

libsf2player_la_SOURCES = sf2_player.cpp sf2_player.h patches_dialog.cpp patches_dialog.h ui_patches_dialog.h
libsf2player_la_LDFLAGS = -L /usr/lib -lfluidsynth

$(libsf2player_la_SOURCES): ./embedded_resources.h
