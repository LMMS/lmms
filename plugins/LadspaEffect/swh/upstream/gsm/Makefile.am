LIBTOOL=libtool
RANLIB=ranlib

noinst_HEADERS = gsm.h config.h private.h proto.h unproto.h
noinst_LIBRARIES = libgsm.a

CFILES = add.c decode.c gsm_decode.c gsm_encode.c long_term.c preprocess.c \
	short_term.c code.c gsm_create.c gsm_destroy.c gsm_option.c lpc.c rpe.c table.c

libgsm_a_SOURCES = $(CFILES) $(noinst_HEADERS)

# Disable autoheader. 
AUTOHEADER=echo

