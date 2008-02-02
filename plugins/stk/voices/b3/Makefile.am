AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES =  -I@stk_includes@ -I$(top_srcdir)/include -I$(top_srcdir)/src/lib -I. -I$(top_srcdir)/plugins/stk/voices/include


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="b3"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./b3_model.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES = libb3.la

libb3_la_SOURCES = 	b3_model.cpp 					\
				b3_model.h 					\
				b3_processor.cpp 				\
				b3_processor.h 				\
				b3_instrument.cpp 				\
				b3_instrument.h 				\
				b3_instrument_view.cpp 			\
				b3_instrument_view.h				\
				$(top_srcdir)/plugins/stk/voices/src/stk_model.cpp		\
				stk_model.h



libb3_la_LDFLAGS = -L@stk_lib@ -lstk -Wl,-export-dynamic

$(libb3_la_SOURCES): ./embedded_resources.h
