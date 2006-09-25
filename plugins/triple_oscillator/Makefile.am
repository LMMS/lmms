AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/lib -I.


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="tripleoscillator"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./triple_oscillator.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES= libtripleoscillator.la

libtripleoscillator_la_SOURCES = triple_oscillator.cpp triple_oscillator.h

$(libtripleoscillator_la_SOURCES): ./embedded_resources.h
