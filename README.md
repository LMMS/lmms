This repository is a fork of the LMMS DAW with a work in progress frequency domain (H(s) and H(f)) transfer function plugin.

To make it clear, LMMS was not created by myself, I have just created the fork with / for the plugin. The main fork of LMMS can be found at https://github.com/LMMS/lmms .

So far, the STFT engine and user interface are implemented, and I am currently working on improving the transfer function parser. Additionally, when analysing with the spectrum analyser, there is aliasing.

The goal is to allow users to define arbitrary H(s) and H(f) curves for  spectral processing inside LMMS.

Native Plugins must be compiled within LMMS. The aim is to create a cross platform plugin (to run within LMMS on any platform which LMMS exists on).

LMMS has many powerful effects, but almost all of them work in the time domain.

This plugin introduces something LMMS has never had before, a true frequency domain processor based on the Short Time Fourier Transform (STFT).

Instead of filtering or distorting the signal in the usual way, this effect aims to let the user directly shape the spectrum using a user defined transfer function.

This opens the door to creative processing that is impossible with normal filters.  I don't believe that there is another plugin or VST which does this.

Stereo processing is not yet implemented the plugin currently operates in mono.

I also plan to implement a bode plot. However, I am slowly working out how to program an LMMS plugin and I have had no success yet graphing.

All files created / modified are stored within the Plugins/TransferFunction folder excluding the PluginList.cmake file (within cmake/modules) which was also required to be edited before compilation.
