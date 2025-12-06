This repository is a fork of the LMMS DAW with a work in progress frequency domain (H(s) and H(f)) transfer function plugin.

To make it clear, LMMS was not created by myself, I have just created the fork with / for the plugins. The main fork of LMMS can be found at https://github.com/LMMS/lmms .

Native Plugins must be compiled within LMMS. The aim is to create a cross platform plugin (to run within LMMS on any platform which LMMS exists on).

**Plugin #1: Transfer Function**
This plugin introduces something LMMS has never had before, a true frequency domain processor based on the Short Time Fourier Transform (STFT).

Instead of filtering or distorting the signal in the usual way, this effect aims to let the user directly shape the spectrum using a user defined transfer function.

This opens the door to creative processing that is impossible with normal filters.  I don't believe that there is another plugin or VST which does this.

- The state of the plugin saves to the song file
- Multiple transfer functions can be entered into the text entry
- STFT implemented
- Parsing for H(s) and H(f) implemented
- Bode plot added
- Stereo added
- Presets / examples added to dial - focus on filters, but should be able to do delays, echos etc
- Dial option one to manually enter a transfer function added
- Tested, hasn't crashed so far
- Aliasing fixed
- User interface implemented
- Bode plot for preset 1 is calculated
- Bode plots for presets 2 to 17 are drawn based on secondary copies of the equations stored within TransferFunctionControls.cpp . This means that if TransferFunction.cpp's presets change, these will not match.  I know this not the best way yet as it does not follow single source of truth

To fix: I’ve had some problems with high-pass filtering using common transfer functions. To remove the low end I basically required a brickwall response or an increase in the filter order (exponent in the equation). The high end works fine even with low orders.

All files created / modified are stored within the Plugins/TransferFunction folder excluding the PluginList.cmake file (within cmake/modules) which was also required to be edited before compilation.

**Plugin #2: StepGate**
- Four 16 step sequences (A/B/C/D)
- All steps stored & saved in song file
- Sequences selectable via knob and automation
- Position LED
- Reads LMMS host BPM
- Internal bar alignment
- Pattern speed multiplier 1, 2,4 *
- Hard / soft gate selector
- Swing control
- Stereo
- Built in tempo synced delay with on / off, feedback and step size
- Do we require internal filters in StepGate?

**Plugin #3: Spectral Gate**
- Combines the rhythmic step sequencing of StepGate with the FFT frequency domain processing of the Transfer Function plugin
- Spectral Morphing: Instead of simple volume gating, the effect morphs between two distinct spectral states ("Open" and "Closed")
- Independent controls for the Open and Closed states including selectable presets and custom formulas
- Dual 3-Band EQ
- Math Parser featuring the H(f) text entry system from Plugin #1
- 17 Spectral Presets
- Future idea, FFT size selector
