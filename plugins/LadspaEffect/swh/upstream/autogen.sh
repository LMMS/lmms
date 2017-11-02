#! /bin/sh
autoreconf -i -I m4 || exit 1

test -n "$NOCONFIGURE" || ./configure "$@"

exit 0
