# Spectrum Analyzer plugin

## Overview

This plugin consists of three widgets and back-end code to provide them with required data.

The top-level widget is SaControlDialog. It populates a configuration widget (created dynamically) and instantiates spectrum display widgets. Its main back-end class is SaControls, which holds all configuration values and globally valid constants (e.g. range definitions).

SaSpectrumDisplay and SaWaterfallDisplay show the result of spectrum analysis. Their main back-end class is SaProcessor, which performs FFT analysis on data received from the Analyzer class, which in turn handles the interface with LMMS.


## Changelog

	1.0.1	2019-06-02
		- code style changes
		- added tool-tips
		- use const for unmodified arrays passed to fft_helpers
	1.0.0	2019-04-07
		- initial release
