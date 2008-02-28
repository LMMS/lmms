AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES =  -I@stk_includes@ -I$(top_srcdir)/include -I$(top_srcdir)/src/lib -I. -I$(top_srcdir)/plugins/stk/voices/include


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="bowed"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./bowed_model.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES = libbowed.la

libbowed_la_SOURCES = 	bowed_model.cpp 					\
				bowed_model.h 					\
				bowed_processor.cpp 				\
				bowed_processor.h 				\
				bowed_instrument.cpp 				\
				bowed_instrument.h 				\
				bowed_instrument_view.cpp 			\
				bowed_instrument_view.h				\
				$(top_srcdir)/plugins/stk/voices/src/stk_model.cpp		\
				stk_model.h



libbowed_la_LDFLAGS = -L@stk_lib@ -lstk -Wl,-export-dynamic

$(libbowed_la_SOURCES): ./embedded_resources.h
