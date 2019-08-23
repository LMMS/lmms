# Spectrum Analyzer plugin

## Overview

This plugin consists of three widgets and back-end code to provide them with required data.

The top-level widget is SaControlDialog. It populates configuration widgets (created dynamically) and instantiates spectrum display widgets. Its main back-end class is SaControls, which holds all configuration values and globally valid constants (e.g. range definitions).

SaSpectrumView and SaWaterfallView show the result of spectrum analysis. Their main back-end class is SaProcessor, which performs FFT analysis on data received from the Analyzer class, which in turn handles the interface with LMMS.


## Changelog
	1.1.0	2019-08-29
		- advanced config: expose hidden constants to user
		- advanced config: add support for FFT window overlapping
		- waterfall: display at native resolution on high-DPI screens
		- waterfall: add cursor and improve label density
		- FFT: fix normalization so that 0 dBFS matches full-scale sinewave
		- FFT: decouple data acquisition from processing and display
		- FFT: separate lock for reallocation (to avoid some needless waiting)
		- moved ranges and other constants to a separate file
		- debug: better performance measurements
	1.0.3	2019-07-25
		- rename and tweak amplitude ranges based on feedback
	1.0.2	2019-07-12
		- variety of small changes based on code review
	1.0.1	2019-06-02
		- code style changes
		- added tool-tips
		- use const for unmodified arrays passed to fft_helpers
	1.0.0	2019-04-07
		- initial release
