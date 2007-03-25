AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/lib -I.


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="polyb302"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./polyb302.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES = libpolyb302.la

libpolyb302_la_SOURCES = polyb302.cpp polyb302.h

$(libpolyb302_la_SOURCES): ./embedded_resources.h
