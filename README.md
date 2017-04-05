# ![LMMS Logo](http://lmms.sourceforge.net/Lmms_logo.png) LMMS

[![Build status](https://img.shields.io/travis/LMMS/lmms.svg?maxAge=3600)](https://travis-ci.org/LMMS/lmms)
[![Latest stable release](https://img.shields.io/github/release/LMMS/lmms.svg?maxAge=3600)](https://lmms.io/download)
[![Overall downloads on Github](https://img.shields.io/github/downloads/LMMS/lmms/total.svg?maxAge=3600)](https://github.com/LMMS/lmms/releases)
[![Join the chat at Discord](https://img.shields.io/badge/chat-on%20discord-7289DA.svg)](https://discord.gg/5kSc32Z)
[![Localise on transifex](https://img.shields.io/badge/localise-on_transifex-green.svg)](https://www.transifex.com/lmms/lmms/)

What is LMMS?
--------------

LMMS is a free cross-platform alternative to commercial programs like
FL Studio®, which allow you to produce music with your computer. This includes
the creation of melodies and beats, the synthesis and mixing of sounds, and
arranging of samples. You can have fun with your MIDI-keyboard and much more;
all in a user-friendly and modern interface.

[Homepage](https://lmms.io)<br>
[Downloads/Releases](https://github.com/LMMS/lmms/releases)<br>
[Developer Wiki](https://github.com/LMMS/lmms/wiki)<br>
[Artist & User Wiki/Documentation](https://lmms.io/documentation)<br>
[Sound Demos](https://lmms.io/showcase/)<br>
[LMMS Sharing Platform](https://lmms.io/lsp/) Share your songs!

Features
---------

* Song-Editor for composing songs
* A Beat+Bassline-Editor for creating beats and basslines
* An easy-to-use Piano-Roll for editing patterns and melodies
* An FX mixer with unlimited FX channels and arbitrary number of effects
* Many powerful instrument and effect-plugins out of the box
* Full user-defined track-based automation and computer-controlled automation sources
* Compatible with many standards such as SoundFont2, VST(i), LADSPA, GUS Patches, and full MIDI support
* MIDI file importing

Building
---------

See [Compiling LMMS](https://github.com/LMMS/lmms/wiki/Compile) on our
wiki for information on how to build LMMS.

Dependencies
------------

#### Build Toolchain
| Supported | Toolchain | Version | Notes | 
|-----------|-----------|---------|-------| 
| ✔️ | [`cmake`](https://cmake.org/) | [`2.8.9`](../blob/master/CMakeLists.txt#L1) |  [`lmms>=master`](../tree/master) |
| ✔️ | [`cmake`](https://cmake.org/) | [`2.8.7`](../blob/stable-1.2/CMakeLists.txt#L1) | [`lmms<=stable-1.2`](../tree/stable-1.2) |
| ✔️ | [`cmake`](https://cmake.org/) | [`2.4.5`](../blob/stable-1.1/CMakeLists.txt#L1) | [`lmms<=stable-1.1`](../tree/stable-1.1) |

#### Compiler
| Supported | Compiler | Version |
|-----------|----------|---------|
| ✔️ | [`gcc/g++`](http://gcc.gnu.org/) | `c++11` |
| ✔️ | [`clang`](http://gcc.gnu.org/) |  |
| ✔️ | [`mingw-w64`](https://sourceforge.net/projects/mingw-w64/) |  | 
| ❌  | [`msvc++`](https://visualstudio.com/vs/cplusplus/) | Not yet supported |

#### Libraries
| Required | Library | Version | Description | 
|----------|---------|---------|-------------|
| ✔️ | [`Qt4`](http://qt.io) | `>=4.3.0` | Qt framework with devel-files (4.4.x recommended) | 
| ✔️ | [`Qt5`](http://qt.io) | `>=5.0.0` | Replaces Qt4, see [Using Qt5](#using-qt5) below. |
| ✔️ | [`libsndfile`](http://www.mega-nerd.com/libsndfile/) |  | Reading and writing sound files |
| ✔️ | [`fftw3`](http://www.fftw.org/) | | Fast fourier transform computing library |
|   | [`libvorbis`](http://xiph.org/vorbis/) | | Audio encoding library |
|   | [`libsamplerate`](http://www.mega-nerd.com/SRC/) | `>=0.1.7` | Audio sample rate converter |
|   | [`libogg`](http://xiph.org/ogg/) | | Multimedia container format |
|   | [`wine`](http://www.winehq.org/) | | Windows-on-Unix (needed for VST support) |
|   | [`libstk`](http://ccrma.stanford.edu/software/stk/) | | Signal processing and algorithmic synthesis library |
|   | [`libfluidsynth`](http://fluidsynth.sourceforge.net/) | | SoundFont synthesis library |
|   | [`fltk`](http://www.fltk.org/) |  | Lightweight GUI library (needed by ZynAddSubFX) |
|   | [`jack`](http://jackaudio.org/)|  | Software and hardware audio routing |
|   | [`sdl`](http://www.libsdl.org/)|  | Audio interface library\* |
|   | [`alsa`](http://www.alsa-project.org/) |  | Audio interface library\* |
|   | [`libportaudio`](http://www.portaudio.com/)|  | Audio interface library\* | 
|   | [`libsoundio`](http://libsound.io/)|  | Audio interface library* |

   > \*One one or more required for audio playback


Join LMMS-development
----------------------

If you are interested in LMMS, its programming, artwork, testing, writing demo
songs, (and improving this README...) or something like that, you're welcome
to participate in the development of LMMS!

Information about what you can do and how can be found in the
[wiki](https://github.com/LMMS/lmms/wiki).

Before coding a new big feature, please _always_
[file an issue](https://github.com/LMMS/lmms/issues/new) for your idea and
suggestions about your feature and about the intended implementation on GitHub
or post to the LMMS developers mailinglist (lmms-devel@lists.sourceforge.net)
and wait for replies! Maybe there are different ideas, improvements, hints or
maybe your feature is not welcome/needed at the moment.
