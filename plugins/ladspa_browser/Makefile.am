AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/lib -I.


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="ladspa_browser"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES =	./ladspa_browser.moc		\
		./ladspa_description.moc	\
		./ladspa_port_dialog.moc

BUILT_SOURCES = $(MOC_FILES) ./embedded_resources.h
EMBEDDED_RESOURCES = $(wildcard *png)

./embedded_resources.h: $(EMBEDDED_RESOURCES)
	$(BIN2RES) $(EMBEDDED_RESOURCES) > $@

EXTRA_DIST = $(EMBEDDED_RESOURCES)


CLEANFILES = $(MOC_FILES) ./embedded_resources.h



pkglib_LTLIBRARIES = libladspa_browser.la

libladspa_browser_la_SOURCES =	ladspa_browser.cpp	\
				ladspa_browser.h	\
				ladspa_description.cpp	\
				ladspa_description.h	\
				ladspa_port_dialog.cpp	\
				ladspa_port_dialog.h

$(libladspa_browser_la_SOURCES): ./embedded_resources.h
