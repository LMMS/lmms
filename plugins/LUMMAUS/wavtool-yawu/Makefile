# Fake Makefile, acturally calls build/Makefile

.PHONY: all install clean

all:
	$(MAKE) -C build all

install:
	$(MAKE) -C build install

clean:
	$(MAKE) -C build clean

test:
	$(MAKE) -C build test
