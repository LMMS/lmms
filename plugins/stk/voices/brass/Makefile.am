AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES =  -I@stk_includes@ -I$(top_srcdir)/include -I$(top_srcdir)/src/lib -I. -I$(top_srcdir)/plugins/stk/voices/include


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="brass"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./brass_model.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES = libbrass.la

libbrass_la_SOURCES = 	brass_model.cpp 					\
				brass_model.h 					\
				brass_processor.cpp 				\
				brass_processor.h 				\
				brass_instrument.cpp 				\
				brass_instrument.h 				\
				brass_instrument_view.cpp 			\
				brass_instrument_view.h				\
				$(top_srcdir)/plugins/stk/voices/src/stk_model.cpp		\
				stk_model.h



libbrass_la_LDFLAGS = -L@stk_lib@ -lstk -Wl,-export-dynamic

$(libbrass_la_SOURCES): ./embedded_resources.h
