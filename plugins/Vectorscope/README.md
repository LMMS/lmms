# Vectorscope plugin

## Overview

Vectorscope is a simple stereo field visualizer. Samples are plotted into a graph, with left and right channels providing the coordinates. Previously drawn samples quickly fade away and are continuously replaced by new samples, creating a real-time plot of the most recently played samples.

Similar to other effect plugins, the top-level widget is VecControlDialog. It displays configuration knobs and the main VectorView widget. The back-end configuration class is VecControls, which holds all models and configuration values.

VectorView computes and shows the plot. It gets data for processing from the Vectorscope class, which handles the interface with LMMS. In order to avoid any stalling of the realtime-sensitive audio thread, data are exchanged through a lockless ring buffer.

## Changelog

	1.0.0	2019-11-21
		- initial release
