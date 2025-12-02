This repository is a fork of the LMMS DAW with a work in progress frequency domain **transfer function** plugin.

To make i clear, LMMS was not created by myself, I have just created the fork with / for the plugin.

So far, the STFT engine and user interface are implemented, and I am currently working on the transfer function parser.

The goal is to allow users to define arbitrary H(ω) curves for creative spectral processing inside LMMS.

Native Plugins must be compiled within LMMS. The aim is to create a cross platform plugin.

LMMS has many powerful effects, but almost all of them work in the **time domain**.

This plugin introduces something LMMS has never had before, a **true frequency-domain processor** based on the Short-Time Fourier Transform (STFT).

Instead of filtering or distorting the signal in the usual way, this effect lets you directly shape the spectrum using a user-defined **transfer function H(ω)**.

This opens the door to creative processing that is impossible with normal filters.

This plugin is designed as an experiment and a new DSP tool that gives LMMS users a level of spectral control normally found only in high end academic software. I don't believe that there is another plugin or VST which does this.

Stereo processing is not yet implemented the plugin currently operates in mono.

All files created / modified are stored within the Plugins/TransferFunction folder excluding the PluginList.cmake file (within cmake/modules) which was also required to be edited before compilation.
