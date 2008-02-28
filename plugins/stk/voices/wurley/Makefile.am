AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES =  -I@stk_includes@ -I$(top_srcdir)/include -I$(top_srcdir)/src/lib -I. -I$(top_srcdir)/plugins/stk/voices/include


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="wurley"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./wurley_model.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES = libwurley.la

libwurley_la_SOURCES = 	wurley_model.cpp 					\
				wurley_model.h 					\
				wurley_processor.cpp 				\
				wurley_processor.h 				\
				wurley_instrument.cpp 				\
				wurley_instrument.h 				\
				wurley_instrument_view.cpp 			\
				wurley_instrument_view.h				\
				$(top_srcdir)/plugins/stk/voices/src/stk_model.cpp		\
				stk_model.h



libwurley_la_LDFLAGS = -L@stk_lib@ -lstk -Wl,-export-dynamic

$(libwurley_la_SOURCES): ./embedded_resources.h
