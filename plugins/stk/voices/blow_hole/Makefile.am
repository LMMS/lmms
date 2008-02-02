AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES =  -I@stk_includes@ -I$(top_srcdir)/include -I$(top_srcdir)/src/lib -I. -I$(top_srcdir)/plugins/stk/voices/include


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="blowhole"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./blow_hole_model.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES = libblowhole.la

libblowhole_la_SOURCES = 	blow_hole_model.cpp 					\
				blow_hole_model.h 					\
				blow_hole_processor.cpp 				\
				blow_hole_processor.h 				\
				blow_hole_instrument.cpp 				\
				blow_hole_instrument.h 				\
				blow_hole_instrument_view.cpp 			\
				blow_hole_instrument_view.h				\
				$(top_srcdir)/plugins/stk/voices/src/stk_model.cpp		\
				stk_model.h



libblowhole_la_LDFLAGS = -L@stk_lib@ -lstk -Wl,-export-dynamic

$(libblowhole_la_SOURCES): ./embedded_resources.h
