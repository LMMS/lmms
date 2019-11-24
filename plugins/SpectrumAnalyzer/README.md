# Spectrum Analyzer plugin

## Overview

This plugin consists of three widgets and back-end code to provide them with required data.

The top-level widget is `SaControlDialog`. It populates configuration widgets (created dynamically) and instantiates spectrum display widgets. Its main back-end class is `SaControls`, which holds all configuration values.

`SaSpectrumView` and `SaWaterfallView` widgets show the result of spectrum analysis. Their main back-end class is `SaProcessor`, which performs FFT analysis on data received from the `Analyzer` class, which in turn handles the interface with LMMS.

## Threads

The Spectrum Analyzer is involved in three different threads:
 - **Effect mixer thread**: periodically calls `Analyzer::processAudioBuffer()` to provide the plugin with more data. This thread is real-time sensitive -- any latency spikes can potentially cause interruptions in the audio stream. For this reason, `Analyzer::processAudioBuffer()` must finish as fast as possible and must not call any functions that could cause it to be delayed for unpredictable amount of time. A lock-less ring buffer is used to safely feed data to the FFT analysis thread without risking any latency spikes due to a shared mutex being unavailable at the time of writing.
 - **FFT analysis thread**: a standalone thread formed by the `SaProcessor::analyze()` function. Takes in data from the ring buffer, performs FFT analysis and prepares results for display. This thread is not real-time sensitive but excessive locking is discouraged to maintain good performance.
 - **GUI thread**: periodically triggers `paintEvent()` of all Qt widgets, including `SaSpectrumView` and `SaWaterfallView`. While it is not as sensitive to latency spikes as the effect mixer thread, the `paintEvent()`s appear to be called sequentially and the execution time of each widget therefore adds to the total time needed to complete one full refresh cycle. This means the maximum frame rate of the Qt GUI will be limited to `1 / total_execution_time`. Good performance of the `paintEvent()` functions should be therefore kept in mind.


## Changelog
	1.1.2	2019-11-18
		- waterfall is no longer cut short when width limit is reached
		- various small tweaks based on final review
	1.1.1	2019-10-13
		- improved interface for accessing SaProcessor private data
		- readme file update
		- other small improvements based on reviews
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
		- various performance optimizations
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
