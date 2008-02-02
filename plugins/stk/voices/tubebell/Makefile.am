AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES =  -I@stk_includes@ -I$(top_srcdir)/include -I$(top_srcdir)/src/lib -I. -I$(top_srcdir)/plugins/stk/voices/include


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="tubebell"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = ./tubebell_model.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES = libtubebell.la

libtubebell_la_SOURCES = 	tubebell_model.cpp 					\
				tubebell_model.h 					\
				tubebell_processor.cpp 				\
				tubebell_processor.h 				\
				tubebell_instrument.cpp 				\
				tubebell_instrument.h 				\
				tubebell_instrument_view.cpp 			\
				tubebell_instrument_view.h				\
				$(top_srcdir)/plugins/stk/voices/src/stk_model.cpp		\
				stk_model.h



libtubebell_la_LDFLAGS = -L@stk_lib@ -lstk -Wl,-export-dynamic

$(libtubebell_la_SOURCES): ./embedded_resources.h
