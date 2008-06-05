AUTOMAKE_OPTIONS = foreign 1.4


AM_CFLAGS := $(AM_CFLAGS) -I../../../include -I. -O3 -Wall -fomit-frame-pointer -fno-strict-aliasing -fstrength-reduce -funroll-loops -ffast-math -c -fPIC -DPIC

DEST = $(DESTDIR)$(libdir)/$(PACKAGE)/ladspa

all: $(PLUGINS)

if BUILD_WIN32
SHARED_EXT=dll
LINKFLAGS=
else
SHARED_EXT=so
LINKFLAGS=-nostartfiles
endif

PLUGINS	=	tap_autopan.$(SHARED_EXT) \
		tap_chorusflanger.$(SHARED_EXT) \
		tap_deesser.$(SHARED_EXT) \
		tap_dynamics_m.$(SHARED_EXT) \
		tap_dynamics_st.$(SHARED_EXT) \
		tap_eq.$(SHARED_EXT) \
		tap_eqbw.$(SHARED_EXT) \
		tap_doubler.$(SHARED_EXT) \
		tap_pinknoise.$(SHARED_EXT) \
		tap_pitch.$(SHARED_EXT) \
		tap_reflector.$(SHARED_EXT) \
		tap_reverb.$(SHARED_EXT) \
		tap_rotspeak.$(SHARED_EXT) \
		tap_limiter.$(SHARED_EXT) \
		tap_sigmoid.$(SHARED_EXT) \
		tap_echo.$(SHARED_EXT) \
		tap_tremolo.$(SHARED_EXT) \
		tap_tubewarmth.$(SHARED_EXT) \
		tap_vibrato.$(SHARED_EXT)


LDFLAGS		= $(LINKFLAGS) -shared -Wl,-no-undefined -Wl,-Bsymbolic -lm

%.$(SHARED_EXT): %.c
	$(CC) $(AM_CFLAGS) -c $< -o $<.o
	$(CC) $(LDFLAGS) $<.o -o $@

clean:
	rm -f *.c.o *.$(SHARED_EXT)


install: all
	install -d $(DEST)
	install -m 644 $(PLUGINS) $(DEST)

uninstall:
	-rm $(DEST)/tap_*.$(SHARED_EXT)

all: $(PLUGINS)

EXTRA_DIST = README COPYING CREDITS $(wildcard tap_*.c) $(wildcard tap_*.h) ladspa.h
