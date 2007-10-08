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

pkglib_PROGRAMS = lvsl_server
lvsl_server_SOURCES = lvsl_server.cpp communication.h
lvsl_server_CPPFLAGS = $(AM_CPPFLAGS) -I${prefix}/include/wine/windows
lvsl_server_CPPFLAGS += -I/usr/include/wine/windows
lvsl_server_LINK = wineg++ -mwindows -lpthread -o $(pkglib_PROGRAMS)
nobase_pkglib_DATA = $(pkglib_PROGRAMS).exe.so

if AMD64_BUILD
lvsl_server_CXXFLAGS = -m32 $(AM_CXXFLAGS)
lvsl_server_LINK += -m32 -Wb,--as-cmd="as --32",--ld-cmd="ld -melf_i386" \
	-L/usr/lib32
endif

CLEANFILES = $(MOC_FILES) $(nobase_pkglib_DATA)

install-exec-hook:
	cd $(DESTDIR)$(pkglibdir) ; \
	strip $(nobase_pkglib_DATA)

