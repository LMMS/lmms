AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES =  -I@stk_includes@ -I$(top_srcdir)/include -I$(top_srcdir)/src/lib -I. -I$(top_srcdir)/plugins/stk/voices/include


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="moog"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./moog_model.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES = libmoog.la

libmoog_la_SOURCES = 	moog_model.cpp 					\
				moog_model.h 					\
				moog_processor.cpp 				\
				moog_processor.h 				\
				moog_instrument.cpp 				\
				moog_instrument.h 				\
				moog_instrument_view.cpp 			\
				moog_instrument_view.h				\
				$(top_srcdir)/plugins/stk/voices/src/stk_model.cpp		\
				stk_model.h



libmoog_la_LDFLAGS = -L@stk_lib@ -lstk -Wl,-export-dynamic

$(libmoog_la_SOURCES): ./embedded_resources.h
