AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/lib


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="bassbooster"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./bassbooster_controls.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES= libbassbooster.la

libbassbooster_la_SOURCES =	bass_booster.cpp		\
				bass_booster.h			\
				bassbooster_controls.cpp	\
				bassbooster_controls.h		\
				bassbooster_control_dialog.cpp	\
				bassbooster_control_dialog.h

$(libbassbooster_la_SOURCES): ./embedded_resources.h
