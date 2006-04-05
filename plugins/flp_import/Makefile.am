AUTOMAKE_OPTIONS = foreign 1.4


INCLUDES = -I$(top_srcdir)/include -I. -I./unrtf

EXTRA_DIST = $(wildcard unrtf/*.c unrtf/*.h)

AM_CXXFLAGS := $(AM_CXXFLAGS) $(QT_CXXFLAGS) -DPLUGIN_NAME="flpimport"


pkglib_LTLIBRARIES= libflpimport.la

libflpimport_la_SOURCES = flp_import.cpp flp_import.h
