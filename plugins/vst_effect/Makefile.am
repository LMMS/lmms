AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/gui -I. -I../vst_base/


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="vsteffect"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./vst_effect_controls.moc


BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES = libvsteffect.la

libvsteffect_la_SOURCES =	vst_effect.cpp 			\
				vst_effect_controls.cpp		\
				vst_effect_control_dialog.cpp	\
				vst_subplugin_features.cpp	\
				vst_subplugin_features.h	\
				vst_effect.h			\
				vst_effect_controls.h		\
				vst_effect_control_dialog.h
libvsteffect_la_LIBADD = -L../vst_base -lvstbase

$(libvsteffect_la_SOURCES): ./embedded_resources.h
