AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES = -I/usr/include/festival -I/usr/include/speech_tools	\
	-I$(top_srcdir)/include -I$(top_srcdir)/src/lib -I.


CXXFLAGS += -Wno-non-template-friend

AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="singerbot"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./singerbot.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES = libsingerbot.la

libsingerbot_la_SOURCES = singerbot.cpp singerbot.h
libsingerbot_la_LDFLAGS = -lFestival -lestools -lestbase -lesd -lncurses

$(libsingerbot_la_SOURCES): ./embedded_resources.h
