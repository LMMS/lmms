AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES =  -I@stk_includes@ -I$(top_srcdir)/include -I$(top_srcdir)/src/lib -I. -I$(top_srcdir)/plugins/stk/voices/include


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="resonate"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./resonate_model.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES = libresonate.la

libresonate_la_SOURCES = 	resonate_model.cpp 					\
				resonate_model.h 					\
				resonate_processor.cpp 				\
				resonate_processor.h 				\
				resonate_instrument.cpp 				\
				resonate_instrument.h 				\
				resonate_instrument_view.cpp 			\
				resonate_instrument_view.h				\
				$(top_srcdir)/plugins/stk/voices/src/stk_model.cpp		\
				stk_model.h



libresonate_la_LDFLAGS = -L@stk_lib@ -lstk -Wl,-export-dynamic

$(libresonate_la_SOURCES): ./embedded_resources.h
