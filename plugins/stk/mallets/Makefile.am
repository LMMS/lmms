AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES = -I@stk_includes@ -I$(top_srcdir)/include -I$(top_srcdir)/src/lib -I.


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="malletsstk"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./mallets.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES= libmalletsstk.la

libmalletsstk_la_SOURCES = mallets.cpp mallets.h 
libmalletsstk_la_LDFLAGS = -L@stk_lib@ -lstk

$(libmalletsstk_la_SOURCES): ./embedded_resources.h
