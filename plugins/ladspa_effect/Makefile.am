AUTOMAKE_OPTIONS = foreign 1.4

if SHIP_CAPS
SUBDIRS = caps
endif


INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/gui -I.


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="ladspaeffect"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./ladspa_effect.moc ./ladspa_controls.moc ./ladspa_control_dialog.moc


BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES = libladspaeffect.la

libladspaeffect_la_SOURCES =	ladspa_effect.cpp 		\
				ladspa_controls.cpp 		\
				ladspa_control_dialog.cpp 	\
				ladspa_subplugin_features.cpp	\
				ladspa_effect.h			\
				ladspa_controls.h		\
				ladspa_control_dialog.h		\
				ladspa_subplugin_features.h

$(libladspaeffect_la_SOURCES): ./embedded_resources.h
