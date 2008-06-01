AUTOMAKE_OPTIONS = foreign 1.4

PLUGIN_NAME = audiofileprocessor


INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/gui -I.


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="$(PLUGIN_NAME)"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./audio_file_processor.moc

if BUILD_WIN32
DLL=$(PLUGIN_NAME).dll

$(DLL): lib$(PLUGIN_NAME).la
	$(CXX) *.o -L$(top_srcdir) -llmms-imp -shared -Wl,-no-undefined -o $@ $(QT_LDADD)

install-exec-hook: $(DLL)
	$(STRIP) $<
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


CLEANFILES = $(MOC_FILES) ./embedded_resources.h $(DLL)


pkglib_LTLIBRARIES = libaudiofileprocessor.la

libaudiofileprocessor_la_SOURCES = audio_file_processor.cpp audio_file_processor.h

$(libaudiofileprocessor_la_SOURCES): ./embedded_resources.h
