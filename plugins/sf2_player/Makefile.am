AUTOMAKE_OPTIONS = foreign 1.4

PLUGIN_NAME = sf2player


INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/gui -I.


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="$(PLUGIN_NAME)"


%.moc: ./%.h
	$(MOC) -o $@ $<

ui_%.h: ./%.ui
	$(UIC) -o $@ $<


MOC_FILES = ./sf2_player.moc ./patches_dialog.moc

UIC_FILES = ./ui_patches_dialog.h

if BUILD_WIN32
DLL=$(PLUGIN_NAME).dll

$(DLL): lib$(PLUGIN_NAME).la
	$(CXX) *.o -L$(top_srcdir) -llmms-imp -shared -Wl,-no-undefined -o $@ $(QT_LDADD) -lfluidsynth-1 && $(STRIP) $@

install-exec-hook: $(DLL)
	cp $< $(pkglibdir)
	rm $(pkglibdir)/lib$(PLUGIN_NAME).a $(pkglibdir)/lib$(PLUGIN_NAME).la
else
DLL=
endif

BUILT_SOURCES = $(MOC_FILES) $(UIC_FILES) ./embedded_resources.h $(DLL)
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ${UIC_FILES} ./embedded_resources.h



pkglib_LTLIBRARIES = libsf2player.la

libsf2player_la_SOURCES = sf2_player.cpp sf2_player.h patches_dialog.cpp patches_dialog.h patches_dialog.ui
libsf2player_la_LDFLAGS = -L@fluidsynth_lib@ -lfluidsynth

$(libsf2player_la_SOURCES): ./embedded_resources.h
