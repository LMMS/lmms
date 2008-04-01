AUTOMAKE_OPTIONS = foreign 1.4


AM_CXXFLAGS := $(AM_CXXFLAGS) -O2 -I../../../include

DEST = $(DESTDIR)$(libdir)/$(PACKAGE)/ladspa
SOURCES = $(wildcard *.cc)
HDRS = $(wildcard *.h) $(wildcard dsp/*.h)
OBJECTS	= $(SOURCES:.cc=.o) 

caps.so: $(OBJECTS) depend
	$(CXX) -nostartfiles $(AM_CXXFLAGS) -shared -o $@ $(OBJECTS)

clean:
	rm -f *.o *.so *.s depend

install: all
#	strip caps.so
	install -d $(DEST)
	install -m 644 caps.so $(DEST)

uninstall:
	-rm $(DEST)/caps.so

depend: $(SOURCES)
	$(CXX) -MM $(AM_CXXFLAGS) $(SOURCES) > depend

.cc.o:
	$(CXX) $(CXXFLAGS) $(AM_CXXFLAGS) -c $<

all: caps.so

EXTRA_DIST = $(SOURCES) $(HDRS) $(wildcard README*)
