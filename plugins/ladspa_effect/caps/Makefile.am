AUTOMAKE_OPTIONS = foreign 1.4


AM_CXXFLAGS := $(AM_CXXFLAGS) -O2 -I../../../include

DEST = $(DESTDIR)$(libdir)/$(PACKAGE)/ladspa
SOURCES = $(wildcard *.cc)
HDRS = $(wildcard *.h) $(wildcard dsp/*.h)
OBJECTS	= $(SOURCES:.cc=.o) 

if BUILD_WIN32
CAPS_SHARED=caps.dll
CAPS_LINKFLAGS=
else
CAPS_SHARED=caps.so
CAPS_LINKFLAGS=-nostartfiles
endif

$(CAPS_SHARED): $(OBJECTS) depend
	$(CXX) $(AM_CXXFLAGS) $(CAPS_LINKFLAGS) -shared -Wl,-no-undefined -o $@ $(OBJECTS)

clean:
	rm -f *.o *.so *.s *.dll depend

install: all
#	strip caps.so
	install -d $(DEST)
	install -m 644 $(CAPS_SHARED) $(DEST)

uninstall:
	-rm $(DEST)/caps.so

depend: $(SOURCES)
	$(CXX) -MM $(AM_CXXFLAGS) $(SOURCES) > depend

.cc.o:
	$(CXX) $(CXXFLAGS) $(AM_CXXFLAGS) -c $<

all: $(CAPS_SHARED)

EXTRA_DIST = $(SOURCES) $(HDRS) $(wildcard README*)
