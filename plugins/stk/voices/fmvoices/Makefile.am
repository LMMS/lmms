AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES =  -I@stk_includes@ -I$(top_srcdir)/include -I$(top_srcdir)/src/lib -I. -I$(top_srcdir)/plugins/stk/voices/include


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="fmvoices"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./fmvoices_model.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES = libfmvoices.la

libfmvoices_la_SOURCES = 	fmvoices_model.cpp 					\
				fmvoices_model.h 					\
				fmvoices_processor.cpp 				\
				fmvoices_processor.h 				\
				fmvoices_instrument.cpp 				\
				fmvoices_instrument.h 				\
				fmvoices_instrument_view.cpp 			\
				fmvoices_instrument_view.h				\
				$(top_srcdir)/plugins/stk/voices/src/stk_model.cpp		\
				stk_model.h



libfmvoices_la_LDFLAGS = -L@stk_lib@ -lstk -Wl,-export-dynamic

$(libfmvoices_la_SOURCES): ./embedded_resources.h
