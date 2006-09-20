AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES = -I$(top_srcdir)/include -I.


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="ladspabase"


%.moc: ./%.h
	$(MOC) -o $@ $<


MOC_FILES = 	./ladspa_control.moc			\
		./ladspa_port_dialog.moc		\
		./ladspa_subplugin_features.moc

BUILT_SOURCES = $(MOC_FILES)


CLEANFILES = $(MOC_FILES)



pkglib_LTLIBRARIES= libladspabase.la

libladspabase_la_SOURCES =	ladspa_base.cpp			\
				ladspa_port_dialog.cpp		\
				ladspa_subplugin_features.cpp	\
				ladspa_2_lmms.cpp		\
				ladspa_manager.cpp		\
				ladspa_control.cpp		\
				ladspa_manager.h		\
				ladspa_2_lmms.h			\
				ladspa_control.h		\
				ladspa_port_dialog.h		\
				ladspa_subplugin_features.h	\
				ladspa_base.h

