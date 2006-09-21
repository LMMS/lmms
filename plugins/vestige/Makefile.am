AUTOMAKE_OPTIONS = foreign 1.4

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/lib -I. -I../vst_base


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="vestige"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./vestige.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(top_builddir)/buildtools/bin2res $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)



pkglib_LTLIBRARIES = libvestige.la

libvestige_la_SOURCES = vestige.cpp vestige.h
libvestige_la_LIBADD = -L../vst_base -lvstbase

$(libvestige_la_SOURCES): ./embedded_resources.h 

CLEANFILES = $(MOC_FILES) ./embedded_resources.h

