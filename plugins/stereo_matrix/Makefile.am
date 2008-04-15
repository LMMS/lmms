AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/gui


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="stereomatrix"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./stereomatrix_controls.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES= libstereomatrix.la

libstereomatrix_la_SOURCES =	stereo_matrix.cpp		\
				stereo_matrix.h			\
				stereomatrix_control_dialog.cpp	\
				stereomatrix_control_dialog.h	\
				stereomatrix_controls.cpp	\
				stereomatrix_controls.h

$(libstereomatrix_la_SOURCES): ./embedded_resources.h
