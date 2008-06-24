AUTOMAKE_OPTIONS = foreign 1.4

PLUGIN_NAME = spectrumanalyzer


INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/gui


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="$(PLUGIN_NAME)"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./spectrumanalyzer_controls.moc

if BUILD_WIN32
DLL=$(PLUGIN_NAME).dll

$(DLL): lib$(PLUGIN_NAME).la
	$(CXX) *.o -L$(top_srcdir) -llmms-imp -shared -Wl,-no-undefined -o $@ $(QT_LDADD) -lfftw3f-3 && $(STRIP) $@

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



pkglib_LTLIBRARIES = libspectrumanalyzer.la

libspectrumanalyzer_la_SOURCES =	spectrum_analyzer.cpp			\
					spectrum_analyzer.h			\
					spectrumanalyzer_controls.cpp		\
					spectrumanalyzer_controls.h		\
					spectrumanalyzer_control_dialog.cpp	\
					spectrumanalyzer_control_dialog.h
if BUILD_WIN32
libspectrumanalyzer_la_LDFLAGS = -lfftw3f-3
else
libspectrumanalyzer_la_LDFLAGS = -lfftw3f
endif

$(libspectrumanalyzer_la_SOURCES): ./embedded_resources.h
