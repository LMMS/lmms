AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/lib -I.


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="vibedstrings"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./vibed.moc ./nine_button_selector.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES= libvibedstrings.la

libvibedstrings_la_SOURCES = vibed.cpp \
                             vibed.h \
                             vibrating_string.cpp \
                             vibrating_string.h \
			     nine_button_selector.cpp \
			     nine_button_selector.h \
                             string_container.cpp \
                             string_container.h

$(libvibedstrings_la_SOURCES): ./embedded_resources.h
