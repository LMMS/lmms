AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES =  -I@stk_includes@ -I$(top_srcdir)/include -I$(top_srcdir)/src/lib -I. -I$(top_srcdir)/plugins/stk/voices/include


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="flute"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./flute_model.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES = libflute.la

libflute_la_SOURCES = 	flute_model.cpp 					\
				flute_model.h 					\
				flute_processor.cpp 				\
				flute_processor.h 				\
				flute_instrument.cpp 				\
				flute_instrument.h 				\
				flute_instrument_view.cpp 			\
				flute_instrument_view.h				\
				$(top_srcdir)/plugins/stk/voices/src/stk_model.cpp		\
				stk_model.h



libflute_la_LDFLAGS = -L@stk_lib@ -lstk -Wl,-export-dynamic

$(libflute_la_SOURCES): ./embedded_resources.h
