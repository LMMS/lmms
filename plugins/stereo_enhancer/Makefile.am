AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/gui


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="stereoenhancer"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./stereoenhancer_controls.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES= libstereoenhancer.la

libstereoenhancer_la_SOURCES =	stereo_enhancer.cpp		\
				stereo_enhancer.h			\
				stereoenhancer_control_dialog.cpp	\
				stereoenhancer_control_dialog.h	\
				stereoenhancer_controls.cpp	\
				stereoenhancer_controls.h

$(libstereoenhancer_la_SOURCES): ./embedded_resources.h
