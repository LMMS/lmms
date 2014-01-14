Jack Interposer
===============

Please see interposer_README for details on the Interposer project.
This document describes how Interposer can be used with LMMS.

General
-------
Jack interposer intercepts the jack_process() call, flagging that we are now
doing RT audio processing. This means that no RT-unsafe calls like malloc(), free()
mutex_lock() etc can be made in this thread.

If such a call is made in this tread, Jack-Interposer raises SIGABRT.
This is very useful when running LMMS in gdb, as it will give is the exact location
of the RT violation.

Compiling LMMS with debug symbols will provide better backtraces.

Usage
-----
Run make to compile: a shared object `jack-interposer.so` is created.
We now LD_PRELOAD the `jack-interposer.so` file, and then run LMMS in gdb.

```
export LD_PRELOAD=`pwd`/jack_interposer.so && gdb lmms
```

Example
-------
Note that the LMMS do be checked must either be found by `which lmms`, or run
the LMMS of choice with a full path. In this example, it is presume that the
LMMS on $PATH is the one to be checked.

```
jack_interposer:$ export LD_PRELOAD=`pwd`/jack_interposer.so && gdb lmms
GNU gdb (GDB) 7.6.1
Copyright (C) 2013 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.  Type "show copying"
and "show warranty" for details.
This GDB was configured as "x86_64-unknown-linux-gnu".
For bug reporting instructions, please see:
<http://www.gnu.org/software/gdb/bugs/>...
Reading symbols from /usr/local/bin/lmms...done.
(gdb) r
Starting program: /usr/local/bin/lmms 
warning: no loadable sections found in added symbol-file system-supplied DSO at 0x7ffff7ffa000
warning: Could not load shared library symbols for linux-vdso.so.1.
Do you need "set solib-search-path" or "set sysroot"?
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/usr/lib/libthread_db.so.1".
[New Thread 0x7fffe298f700 (LWP 28258)]
[New Thread 0x7fffe6b57700 (LWP 28279)]
[New Thread 0x7fffe01cf700 (LWP 28280)]
[New Thread 0x7fffbe8f4700 (LWP 28281)]
[New Thread 0x7fffbddef700 (LWP 28286)]
[New Thread 0x7fffe0098700 (LWP 28287)]
pthread_mutex_lock() is called while in rt section

Program received signal SIGABRT, Aborted.
[Switching to Thread 0x7fffe0098700 (LWP 28287)]
0x00007ffff3d2e3d9 in raise () from /usr/lib/libc.so.6
(gdb) bt
#0  0x00007ffff3d2e3d9 in raise () from /usr/lib/libc.so.6
#1  0x00007ffff3d2f7d8 in abort () from /usr/lib/libc.so.6
#2  0x00007ffff7bdb246 in pthread_mutex_lock () from /home/username/programming/lmms/rttests/jack_interposer/jack_interposer.so
#3  0x00007ffff66690cc in QWaitCondition::wakeAll() () from /usr/lib/libQtCore.so.4
#4  0x00007ffff6665503 in QSemaphore::release(int) () from /usr/lib/libQtCore.so.4
#5  0x00000000004d7378 in read (this=<optimized out>) at /home/username/programming/lmms/include/fifo_buffer.h:65
#6  nextBuffer (this=<optimized out>) at /home/username/programming/lmms/include/Mixer.h:362
#7  AudioDevice::getNextBuffer (this=this@entry=0xb9f180, _ab=0xba24d0) at /home/username/programming/lmms/src/core/audio/AudioDevice.cpp:83
#8  0x00000000004d592e in AudioJack::processCallback (this=0xb9f170, _nframes=128, _udata=<optimized out>)
    at /home/username/programming/lmms/src/core/audio/AudioJack.cpp:390
#9  0x00007ffff7bdbb0b in interposed_process_callback () from /home/username/programming/lmms/rttests/jack_interposer/jack_interposer.so
#10 0x00007ffff59b1c7a in ?? () from /usr/lib/libjack.so.0
#11 0x00007ffff59b0fa8 in ?? () from /usr/lib/libjack.so.0
#12 0x00007ffff59c85c0 in ?? () from /usr/lib/libjack.so.0
#13 0x00007ffff79c40a2 in start_thread () from /usr/lib/libpthread.so.0
#14 0x00007ffff3dde49d in clone () from /usr/lib/libc.so.6
```

It is shown that a mutex is locked during the RT codepath: this needs to be fixed.
