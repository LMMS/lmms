AUTOMAKE_OPTIONS = foreign 1.4

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/lib -I.


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="vestige"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./vestige.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(top_builddir)/buildtools/bin2res $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h xfst.exe.so

pkglib_LTLIBRARIES = libvestige.la

libvestige_la_SOURCES = vestige.cpp vestige.h fstclient.cpp fstclient.h

$(libvestige_la_SOURCES): ./embedded_resources.h 


CC = winegcc
pkglib_PROGRAMS = xfst_server
xfst_server_SOURCES = fstserver.cpp fstcore.c fstserver.h communication.h
xfst_server_LDFLAGS = -mwindows -L/usr/X11R6/lib -lX11 -lpthread -o $(pkglib_PROGRAMS)
xfst_server_LINK = wineg++
nobase_pkglib_DATA = $(pkglib_PROGRAMS).exe.so

