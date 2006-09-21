AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES = -I$(top_srcdir)/include -I.


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="vstbase"

%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./lvsl_client.moc

BUILT_SOURCES = $(MOC_FILES)

pkglib_LTLIBRARIES = libvstbase.la

libvstbase_la_SOURCES =	vst_base.cpp		\
			lvsl_client.cpp		\
			lvsl_client.h

CC = wineg++
pkglib_PROGRAMS = lvsl_server
lvsl_server_SOURCES = lvsl_server.c communication.h
lvsl_server_LDFLAGS = -mwindows -lpthread -o $(pkglib_PROGRAMS)
lvsl_server_LINK = wineg++
nobase_pkglib_DATA = $(pkglib_PROGRAMS).exe.so

CLEANFILES = $(MOC_FILES) $(nobase_pkglib_DATA)

install-exec-hook:
	cd $(DESTDIR)$(pkglibdir) ; \
	strip $(nobase_pkglib_DATA)

