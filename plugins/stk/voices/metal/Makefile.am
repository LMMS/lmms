AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES =  -I@stk_includes@ -I$(top_srcdir)/include -I$(top_srcdir)/src/lib -I. -I$(top_srcdir)/plugins/stk/voices/include


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="metal"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./metal_model.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES = libmetal.la

libmetal_la_SOURCES = 	metal_model.cpp 					\
				metal_model.h 					\
				metal_processor.cpp 				\
				metal_processor.h 				\
				metal_instrument.cpp 				\
				metal_instrument.h 				\
				metal_instrument_view.cpp 			\
				metal_instrument_view.h				\
				$(top_srcdir)/plugins/stk/voices/src/stk_model.cpp		\
				stk_model.h



libmetal_la_LDFLAGS = -L@stk_lib@ -lstk -Wl,-export-dynamic

$(libmetal_la_SOURCES): ./embedded_resources.h
