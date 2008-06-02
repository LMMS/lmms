AUTOMAKE_OPTIONS = foreign 1.4

PLUGIN_NAME = bassbooster


INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/gui


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="$(PLUGIN_NAME)"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./bassbooster_controls.moc

if BUILD_WIN32
DLL=$(PLUGIN_NAME).dll

$(DLL): lib$(PLUGIN_NAME).la
	$(CXX) *.o -L$(top_srcdir) -llmms-imp -shared -Wl,-no-undefined -o $@ $(QT_LDADD) && $(STRIP) $@

install-exec-hook: $(DLL)
	cp $< $(pkglibdir)
	rm $(pkglibdir)/lib$(PLUGIN_NAME).a $(pkglibdir)/lib$(PLUGIN_NAME).la
else
DLL=
endif

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h $(DLL)
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES= libbassbooster.la

libbassbooster_la_SOURCES =	bass_booster.cpp		\
				bass_booster.h			\
				bassbooster_controls.cpp	\
				bassbooster_controls.h		\
				bassbooster_control_dialog.cpp	\
				bassbooster_control_dialog.h

$(libbassbooster_la_SOURCES): ./embedded_resources.h
