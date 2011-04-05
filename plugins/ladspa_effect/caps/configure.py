#! /usr/bin/env python
import os

CFLAGS = []
OSX_LDFLAGS = "-bundle -undefined suppress -flat_namespace"

def we_have_sse():
	try: return 'sse' in open ('/proc/cpuinfo').read().split()
	except: return 0
def we_have_ssse3():
	try: return 'ssse3' in open ('/proc/cpuinfo').read().split()
	except: return 0

def we_think_so_different_dude():
	try: return 'Darwin' == os.popen ('uname -s').read().strip()
	except: return 0

def store():
	f = open ('defines.make', 'w')
	f.write ("_CFLAGS=" + ' '.join (CFLAGS) + "\n")
	if we_think_so_different_dude():
		f.write ("_LDFLAGS=" + OSX_LDFLAGS + "\n")
		f.write ("STRIP = echo\n")
	
if __name__ == '__main__':
	if we_have_sse():
		CFLAGS += ('-msse', '-mfpmath=sse')
	if we_have_ssse3():
		CFLAGS += ('-msse3',)
	store()
