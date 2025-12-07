This repository is a fork of the LMMS DAW with additional Plugins. It was named H(s) as the original extra plugin was for the transfer function.

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
- Spectral Morphing. Instead of simple volume gating, the effect morphs between two distinct spectral states ("Open" and "Closed")
- Independent controls for the Open and Closed states including selectable presets and custom formulas
- Dual 3-Band EQ
- Math Parser featuring the H(f) text entry system from Plugin #1
- 17 Spectral Presets
- Future idea, FFT size selector

 **Plugin #4: Visualiser**
- Introduces visual analysis and artistic rendering to LMMS. Unlike standard spectrum analyzers, this plugin combines scientific audio metering with a "Demoscene" aesthetic
- 32 Selectable Modes ranging from standard Waveforms and Spectrum Analyzers to "Amiga" style Copper Bars, Starfields, and Retro Sunsets
- Stereo FFT Analysis. Uses Fast Fourier Transform to drive frequency-based visuals
- Kraftwerk &  Amiga demo tributes
- Text Customisation with classic Amiga style Sine-Wave Scroller
- Random Cycle Mode. Mode #31 automatically cycles through a random visualisation every 3 seconds
- Scientific Tools including Phase Scope, Complex Plane, Data Stats and Oscilloscope
- State Saving as current mode, text and wiggle settings are saved within the song file
- All rendering is done via QPainter for maximum compatibility across platforms. tested and stable
- All files created are stored within the Plugins/Visualiser folder
- Audio passthrough, use as an effect on channel, instrument or master


