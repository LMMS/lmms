#
# pkgconfig for rtosc
#

prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=${prefix}
libdir=${exec_prefix}
includedir=${prefix}/include

Name: rtosc
Description: rtosc - a realtime safe open sound control serialization and dispatch system
Version: @VERSION_MAJOR@.@VERSION_MINOR@.@VERSION_PATCH@
Libs: -L${libdir} -lrtosc
Cflags: -I${includedir}
