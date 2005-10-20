AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/lib -I.


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="pluckedstringsynth"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./plucked_string_synth.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(top_builddir)/buildtools/bin2res $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES= libpluckedstringsynth.la

libpluckedstringsynth_la_SOURCES = plucked_string_synth.cpp plucked_string_synth.h

$(libpluckedstringsynth_la_SOURCES): ./embedded_resources.h
