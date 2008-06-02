AUTOMAKE_OPTIONS = foreign 1.4

PLUGIN_NAME = flpimport


INCLUDES = -I$(top_srcdir)/include -I. -I./unrtf

EXTRA_DIST = $(wildcard unrtf/*.c unrtf/*.h)

AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="$(PLUGIN_NAME)"

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

BUILT_SOURCES = $(DLL)

pkglib_LTLIBRARIES= libflpimport.la

libflpimport_la_SOURCES = flp_import.cpp flp_import.h
