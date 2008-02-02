AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES =  -I@stk_includes@ -I$(top_srcdir)/include -I$(top_srcdir)/src/lib -I. -I$(top_srcdir)/plugins/stk/voices/include


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="rhodey"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./rhodey_model.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES = librhodey.la

librhodey_la_SOURCES = 	rhodey_model.cpp 					\
				rhodey_model.h 					\
				rhodey_processor.cpp 				\
				rhodey_processor.h 				\
				rhodey_instrument.cpp 				\
				rhodey_instrument.h 				\
				rhodey_instrument_view.cpp 			\
				rhodey_instrument_view.h				\
				$(top_srcdir)/plugins/stk/voices/src/stk_model.cpp		\
				stk_model.h



librhodey_la_LDFLAGS = -L@stk_lib@ -lstk -Wl,-export-dynamic

$(librhodey_la_SOURCES): ./embedded_resources.h
