This repository is a fork of the LMMS DAW with a work in progress frequency domain (H(s) and H(f)) transfer function plugin.

To make it clear, LMMS was not created by myself, I have just created the fork with / for the plugin. The main fork of LMMS can be found at https://github.com/LMMS/lmms .


Native Plugins must be compiled within LMMS. The aim is to create a cross platform plugin (to run within LMMS on any platform which LMMS exists on).

LMMS has many powerful effects, but almost all of them work in the time domain.

This plugin introduces something LMMS has never had before, a true frequency domain processor based on the Short Time Fourier Transform (STFT).

Instead of filtering or distorting the signal in the usual way, this effect aims to let the user directly shape the spectrum using a user defined transfer function.

This opens the door to creative processing that is impossible with normal filters.  I don't believe that there is another plugin or VST which does this.

At this point in time:

- The state of the plugin saves to the song file
- Multiple transfer functions can be entered into the text entry
- STFT implemented
- Parsing for H(s) and H(f) implemented
- Bode plot added
- Stereo added
- Presets / examples added to dial - focus on filters, but should be able to do delays, echos etc- Dial option one to manually enter a transfer function added
- Tested, hasn't crashed so far
- Aliasing fixed
- User interface implemented


That said, I have had some problems with high pass filtering using common transfer functions. I found the only way to filter the low end seems to be with a brick wall. High end, no issues.

All files created / modified are stored within the Plugins/TransferFunction folder excluding the PluginList.cmake file (within cmake/modules) which was also required to be edited before compilation.
