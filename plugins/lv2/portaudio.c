/*
   Copyright 2007-2016 David Robillard <http://drobilla.net>

   Permission to use, copy, modify, and/or distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
   WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
   MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
   ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <math.h>
#include <portaudio.h>

#include "jalv_internal.h"
#include "worker.h"

struct JalvBackend {
	PaStream* stream;
};

static int
pa_process_cb(const void*                     inputs,
              void*                           outputs,
              unsigned long                   nframes,
              const PaStreamCallbackTimeInfo* time,
              PaStreamCallbackFlags           flags,
              void*                           handle)
{
	Jalv* jalv = (Jalv*)handle;

	/* Prepare port buffers */
	uint32_t in_index  = 0;
	uint32_t out_index = 0;
	for (uint32_t i = 0; i < jalv->num_ports; ++i) {
		struct Port* port = &jalv->ports[i];
		if (port->type == TYPE_AUDIO) {
			if (port->flow == FLOW_INPUT) {
				lilv_instance_connect_port(jalv->instance, i, ((float**)inputs)[in_index++]);
			} else if (port->flow == FLOW_OUTPUT) {
				lilv_instance_connect_port(jalv->instance, i, ((float**)outputs)[out_index++]);
			}
		} else if (port->type == TYPE_EVENT && port->flow == FLOW_INPUT) {
			lv2_evbuf_reset(port->evbuf, true);

			if (jalv->request_update) {
				/* Plugin state has changed, request an update */
				const LV2_Atom_Object get = {
					{ sizeof(LV2_Atom_Object_Body), jalv->urids.atom_Object },
					{ 0, jalv->urids.patch_Get } };
				LV2_Evbuf_Iterator iter = lv2_evbuf_begin(port->evbuf);
				lv2_evbuf_write(&iter, 0, 0,
				                get.atom.type, get.atom.size,
				                (const uint8_t*)LV2_ATOM_BODY(&get));
			}
		} else if (port->type == TYPE_EVENT) {
			/* Clear event output for plugin to write to */
			lv2_evbuf_reset(port->evbuf, false);
		}
	}
	jalv->request_update = false;

	/* Run plugin for this cycle */
	const bool send_ui_updates = jalv_run(jalv, nframes);

	/* Deliver UI events */
	for (uint32_t p = 0; p < jalv->num_ports; ++p) {
		struct Port* const port = &jalv->ports[p];
		if (port->flow == FLOW_OUTPUT && port->type == TYPE_EVENT) {
			for (LV2_Evbuf_Iterator i = lv2_evbuf_begin(port->evbuf);
			     lv2_evbuf_is_valid(i);
			     i = lv2_evbuf_next(i)) {
				// Get event from LV2 buffer
				uint32_t frames, subframes, type, size;
				uint8_t* body;
				lv2_evbuf_get(i, &frames, &subframes, &type, &size, &body);

				if (jalv->has_ui && !port->old_api) {
					// Forward event to UI
					jalv_send_to_ui(jalv, p, type, size, body);
				}
			}
		} else if (send_ui_updates &&
		           port->flow == FLOW_OUTPUT && port->type == TYPE_CONTROL) {
			char buf[sizeof(ControlChange) + sizeof(float)];
			ControlChange* ev = (ControlChange*)buf;
			ev->index    = p;
			ev->protocol = 0;
			ev->size     = sizeof(float);
			*(float*)ev->body = port->control;
			if (zix_ring_write(jalv->plugin_events, buf, sizeof(buf))
			    < sizeof(buf)) {
				fprintf(stderr, "Plugin => UI buffer overflow!\n");
			}
		}
	}

	return paContinue;
}

static JalvBackend*
pa_error(const char* msg, PaError err)
{
	fprintf(stderr, "error: %s (%s)\n", msg, Pa_GetErrorText(err));
	Pa_Terminate();
	return NULL;
}

JalvBackend*
jalv_backend_init(Jalv* jalv)
{
	PaStreamParameters inputParameters;
	PaStreamParameters outputParameters;
	PaStream*          stream = NULL;
	PaError            st     = paNoError;

	if ((st = Pa_Initialize())) {
		return pa_error("Failed to initialize audio system", st);
	}

	// Get default input and output devices
	inputParameters.device  = Pa_GetDefaultInputDevice();
	outputParameters.device = Pa_GetDefaultOutputDevice();
	if (inputParameters.device == paNoDevice) {
		return pa_error("No default input device", paDeviceUnavailable);
	} else if (outputParameters.device == paNoDevice) {
		return pa_error("No default output device", paDeviceUnavailable);
	}

	const PaDeviceInfo* in_dev  = Pa_GetDeviceInfo(inputParameters.device);
	const PaDeviceInfo* out_dev = Pa_GetDeviceInfo(outputParameters.device);

	// Count number of input and output audio ports/channels
	inputParameters.channelCount  = 0;
	outputParameters.channelCount = 0;
	for (uint32_t i = 0; i < jalv->num_ports; ++i) {
		if (jalv->ports[i].type == TYPE_AUDIO) {
			if (jalv->ports[i].flow == FLOW_INPUT) {
				++inputParameters.channelCount;
			} else if (jalv->ports[i].flow == FLOW_OUTPUT) {
				++outputParameters.channelCount;
			}
		}
	}

	// Configure audio format
	inputParameters.sampleFormat               = paFloat32|paNonInterleaved;
	inputParameters.suggestedLatency           = in_dev->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo  = NULL;
	outputParameters.sampleFormat              = paFloat32|paNonInterleaved;
	outputParameters.suggestedLatency          = out_dev->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	// Open stream
	if ((st = Pa_OpenStream(
		     &stream,
		     inputParameters.channelCount ? &inputParameters : NULL,
		     outputParameters.channelCount ? &outputParameters : NULL,
		     in_dev->defaultSampleRate,
		     paFramesPerBufferUnspecified,
		     0,
		     pa_process_cb,
		     jalv))) {
		return pa_error("Failed to open audio stream", st);
	}

	// Set audio parameters
	jalv->sample_rate   = in_dev->defaultSampleRate;
	// jalv->block_length  = FIXME
	jalv->midi_buf_size = 4096;

	// Allocate and return opaque backend
	JalvBackend* backend = (JalvBackend*)calloc(1, sizeof(JalvBackend));
	backend->stream = stream;
	return backend;
}

void
jalv_backend_close(Jalv* jalv)
{
	Pa_Terminate();
	free(jalv->backend);
	jalv->backend = NULL;
}

void
jalv_backend_activate(Jalv* jalv)
{
	const int st = Pa_StartStream(jalv->backend->stream);
	if (st != paNoError) {
		fprintf(stderr, "error: Error starting audio stream (%s)\n",
		        Pa_GetErrorText(st));
	}
}

void
jalv_backend_deactivate(Jalv* jalv)
{
	const int st = Pa_CloseStream(jalv->backend->stream);
	if (st != paNoError) {
		fprintf(stderr, "error: Error closing audio stream (%s)\n",
		        Pa_GetErrorText(st));
	}
}

void
jalv_backend_activate_port(Jalv* jalv, uint32_t port_index)
{
	struct Port* const port = &jalv->ports[port_index];
	switch (port->type) {
	case TYPE_CONTROL:
		lilv_instance_connect_port(jalv->instance, port_index, &port->control);
		break;
	default:
		break;
	}
}
