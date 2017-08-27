ZynAddSubFX
-----------
It is a realtime software synthesizer for Linux and Windows with many features. Please see the docs for details.
Copyright (c) 2002-2014 Nasca Octavian Paul and others contributors
e-mail: zynaddsubfx AT yahoo D0T com
ZynAddSubFX is free program and is distributed WITH NO WARRANTY. It is licensed under GNU General Public License version 2 (and only version 2) - see the file COPYING.

    --==## PLEASE SHARE YOUR INSTRUMENTS/MASTER SETTINGS ##==--
              --==## MADE WITH ZynAddSubFX ##==--
    Here is the mailing list where you can share your patches with others:
      http://lists.sourceforge.net/mailman/listinfo/zynaddsubfx-user


The project page is
    http://sourceforge.net/projects/zynaddsubfx
    or
    http://zynaddsubfx.sourceforge.net

ZynAddSubFX is also available on many Internet sites like:
    http://www-ccrma.stanford.edu/planetccrma/software/soundapps.html (Planet CCRMA)
    http://www.hitsquad.com/smm/programs/ZynAddSubFX/
    http://freshmeat.net/projects/zynaddsubfx/
    http://ibiblio.org/pub/Linux/apps/sound/midi/
    or search "ZynAddSubFX" on a search engine (like www.google.com).


Requirements:
-------------
- a fast computer
- Linux or Windows
- FFTW 3.x.x  - necessary for Fast Fourier computations
- MXML-2.5 or more recent library from www.minixml.org
- zlib library from http://www.zlib.org
- (for Linux) OpenSoundSystem (OSS) (if you don't have ALSA, only)
- (for Windows) pthreads, portaudio

Not required, but recommended:
------------------------------
- FLTK 1.x.x (tested with fltk 1.1.0, 1.1.1, 1.1.2,etc.)
- ALSA 0.9.x or later (with OSS emulation, if you don't use JACK)
- JACK
- a VST host for the VST version [For more information see:
  http://www.kvraudio.com/forum/viewtopic.php?t=268277&sid=95be0b6c9909300d566006428bcbb97d]

Compilation:
------------
    For the main program see doc/build.txt.
    To compile the Spliter, run "make" from the "Spliter" directory.
    To compile the Controller, run "make" from the "Controller" directory.

Running on LINUX
----------------
Under linux there are several options for both audio output and MIDI input.
Defaults are set at compile time and the desired backend can be set when starting ZynAddSubFX with the '-I' and '-O' options.
The currently supported backends are:

- Audio Output
    * ALSA (Advanced Linux Sound Architecture)
    * OSS (Open Sound System)
    * JACK (JACK Audio Connection Kit)
    * Port Audio

- MIDI Input
    * ALSA
    * OSS
    * JACK

Running on WINDOWS
------------------
NOTE: At this time only older versions of ZynAddSubFX were compiled on windows
      See older versions on sf.net (ie version 2.2.1)
    If you launch zynaddsubfx.exe and nothing happens, you need pthreadGC.dll  in the same directory (or windows directory). The dll files are distribuited with ZynAddSubFX windows binaries.
    It might be possible that the latency will be very high. If this happens, you have to set the environment variable PA_MIN_LATENCY_MSEC to a value that represents the latency in miliseconds.
    Eg: (in autoexec.bat or launched before running ZynAddSubFX) "set PA_MIN_LATENCY_MSEC=50"
    Warning: if the value is too low, you might encounter severe dropouts on ZynAddSubFX. You'll have to set to a higher value and turn off automated background tasks (like virus scanners, email clients, etc.).
    If you have more cards, you can select the desired card where you can play audio with the environment variable "PA_RECOMMENDED_OUTPUT_DEVICE"
    Eg: "set PA_RECOMMENDED_OUTPUT_DEVICE=1"
    A better way to set all of this, I will put on next versions.


Please send me instruments,banks,master settings,songs(midi+...xmz files) done with ZynAddSubFX. I'll appreciate this.


Have fun! :-)

--The ZynAddSubFX team
