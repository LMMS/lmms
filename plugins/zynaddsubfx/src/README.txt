ZynAddSubFX
-----------
It is a realtime software synthesizer for Linux and Windows with many features. Please see the docs for details.
Copyright (c) 2002-2006 Nasca Octavian Paul and others contribuitors
e-mail: zynaddsubfx AT yahoo D0T com
ZynAddSubFX is free program and is distribuited WITH NO WARRANTY. It is licensed under GNU General Public License version 2 (and only version 2) - see the file COPYING.

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
               - Linux (tested with RedHat 7.2,7.3,etc.) or Windows
               - FFTW 2.x.x (tested with fftw 2.0.5, 2.1.3) - necesary for
  Fast Fourier computations
	       - MXML-2.2 library from http://www.easysw.com/~mike/mxml/
	       - zlib library from http://www.zlib.org - this exists in most linux distributions
               - (for Linux) OpenSoundSystem (OSS) (if you don't have ALSA, only)
	       - (for Windows) pthreads, portaudio

Not requred, but recomanded:
---------------------------

    - FLTK 1.x.x (tested with fltk 1.1.0, 1.1.1, 1.1.2,etc.)
    - ALSA 0.9.x or later (with OSS emulation, if you don't use JACK)
    - JACKit - if you want to use it you must enable compilation for JACK in Makefile.inc
    - a VST host for the VST version

Compilation:
------------
    If you want to compile on Windows, please read compile.win32 file.
    If you don't know how to compile, you may download binaries from Planet CCRMA (see above, on sites).
    First set what should sound input/ouput should use in Makefile.inc in src/ directory.
    Then "make" from the "src/" directory. Hope all goes right. If the compiler complains something about FFTwrapper.h and FFTW library headers(rfftw.h or fftw.h) please read the docs from DSP/FFTwrapper.h .
    To compile the Spliter, run "compile.sh" from the "Spliter" directory. 
    To compile the Controller, run "compile.sh" from the "Controller" directory. 

Running on LINUX
----------------
  *AUDIO OUTPUT
   A) OSS (Open Sound System)
   B) JACK (JACK Audio Connection Kit)
    
  *MIDI INPUT*
    There are 2 possibilites of midi inputs (depends on what you have chosen in Makefile.inc to use - OSS or ALSA).
    A) ALSA (Advenced Linux Sound Architecture)
	1) Launch zynaddsubfx
	2) ZynAddSubFX will make a virtual MIDI port. 
	   You can connect other midi devices (like a real MIDI keyboard, midi sequencers which supports ALSA or virtual keyboard - like vkeybd).
	   To connect, use "aconnect" or "alsa-patch-bay"; usualy the port of ZynAddSubFX is 128:0 or 129:0.
	3) You are ready to play
	   
	   It is possible to use midi sequencer/other software that doesn't supports ALSA with ZynAddSubFX, but this is a bit more complicated. 
    	   Search on Internet for "HOWTO Use MIDI Sequencers With Softsynths" by Frank Barknecht, if you want to do this.


    B) OSS (Open Sound System)
       1) Launch zynaddsubfx
       2) Connect the midi keyboard 
       
       As you have seen the OSS option needs a real midi keyboard. If you don't have it, you can download/install ALSA from www.alsa-project.org

Running on WINDOWS
------------------
    If you launch zynaddsubfx.exe and nothing happens, you need pthreadGC.dll  in the same directory (or windows directory). The dll files are distribuited with zynaddsubfx windows binaries.
    It might be possible that the latency will be very high. If this happens, you have to set the environment variable PA_MIN_LATENCY_MSEC to a value that represents the latency in miliseconds.
	Eg: (in autoexec.bat or launched before running zynaddsubfx) "set PA_MIN_LATENCY_MSEC=50"
    Warning: if the value is too low, you might ecounter severe dropouts on zynaddsubfx. You'll have to set to a higher value and turn off automated background tasks (like virus scanners, email clients, etc.).
    If you have more cards, you can select the desired card where you can play audio with the evironment variable "PA_RECOMMENDED_OUTPUT_DEVICE"
	Eg: "set PA_RECOMMENDED_OUTPUT_DEVICE=1"
    A better way to set all of this, I will put on next versions.

Please send me instruments,banks,master settings,songs(midi+...xmz files) done with ZynAddSubFX. I'll apreciate this.


Have fun! :-)


