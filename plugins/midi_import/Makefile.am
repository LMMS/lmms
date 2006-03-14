AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES = -I$(top_srcdir)/include -I.


AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="midiimport"


pkglib_LTLIBRARIES= libmidiimport.la

libmidiimport_la_SOURCES = midi_import.cpp midi_import.h
