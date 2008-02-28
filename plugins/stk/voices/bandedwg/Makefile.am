AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES =  -I@stk_includes@ -I$(top_srcdir)/include -I$(top_srcdir)/src/lib -I. -I$(top_srcdir)/plugins/stk/voices/include


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="bandedwg"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./bandedwg_model.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES = libbandedwg.la

libbandedwg_la_SOURCES = 	bandedwg_model.cpp 					\
				bandedwg_model.h 					\
				bandedwg_processor.cpp 				\
				bandedwg_processor.h 				\
				bandedwg_instrument.cpp 				\
				bandedwg_instrument.h 				\
				bandedwg_instrument_view.cpp 			\
				bandedwg_instrument_view.h				\
				$(top_srcdir)/plugins/stk/voices/src/stk_model.cpp		\
				stk_model.h



libbandedwg_la_LDFLAGS = -L@stk_lib@ -lstk -Wl,-export-dynamic

$(libbandedwg_la_SOURCES): ./embedded_resources.h
