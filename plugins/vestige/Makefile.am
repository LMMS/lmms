AUTOMAKE_OPTIONS = foreign 1.4

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/lib -I. -I/usr/X11R6/include


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

libvestige_la_SOURCES = vestige.cpp vestige.h lvsl_client.cpp lvsl_client.h

$(libvestige_la_SOURCES): ./embedded_resources.h 



CC = wineg++
pkglib_PROGRAMS = lvsl_server
lvsl_server_SOURCES = lvsl_server.c communication.h
lvsl_server_LDFLAGS = -mwindows -L/usr/X11R6/lib -lX11 -lpthread -o $(pkglib_PROGRAMS)
lvsl_server_LINK = wineg++
nobase_pkglib_DATA = $(pkglib_PROGRAMS).exe.so

CLEANFILES = $(MOC_FILES) ./embedded_resources.h $(nobase_pkglib_DATA)


install-exec-hook:
	cd $(DESTDIR)$(pkglibdir) ; \
	strip $(nobase_pkglib_DATA)

