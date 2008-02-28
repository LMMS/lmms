AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES =  -I@stk_includes@ -I$(top_srcdir)/include -I$(top_srcdir)/src/lib -I. -I$(top_srcdir)/plugins/stk/voices/include


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="percflute"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./percflute_model.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES = libpercflute.la

libpercflute_la_SOURCES = 	percflute_model.cpp 					\
				percflute_model.h 					\
				percflute_processor.cpp 				\
				percflute_processor.h 				\
				percflute_instrument.cpp 				\
				percflute_instrument.h 				\
				percflute_instrument_view.cpp 			\
				percflute_instrument_view.h				\
				$(top_srcdir)/plugins/stk/voices/src/stk_model.cpp		\
				stk_model.h



libpercflute_la_LDFLAGS = -L@stk_lib@ -lstk -Wl,-export-dynamic

$(libpercflute_la_SOURCES): ./embedded_resources.h
