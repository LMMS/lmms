AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES =  -I@stk_includes@ -I$(top_srcdir)/include -I$(top_srcdir)/src/lib -I. -I$(top_srcdir)/plugins/stk/voices/include


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="clarinet"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./clarinet_model.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES = libclarinet.la

libclarinet_la_SOURCES = 	clarinet_model.cpp 					\
				clarinet_model.h 					\
				clarinet_processor.cpp 				\
				clarinet_processor.h 				\
				clarinet_instrument.cpp 				\
				clarinet_instrument.h 				\
				clarinet_instrument_view.cpp 			\
				clarinet_instrument_view.h				\
				$(top_srcdir)/plugins/stk/voices/src/stk_model.cpp		\
				stk_model.h



libclarinet_la_LDFLAGS = -L@stk_lib@ -lstk -Wl,-export-dynamic

$(libclarinet_la_SOURCES): ./embedded_resources.h
