#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include "config.h"
#endif

#ifdef ENABLE_NLS
#include <libintl.h>
#endif

#define         _ISOC9X_SOURCE  1
#define         _ISOC99_SOURCE  1
#define         __USE_ISOC99    1
#define         __USE_ISOC9X    1

#include <math.h>

#include "ladspa.h"

#ifdef WIN32
#define _WINDOWS_DLL_EXPORT_ __declspec(dllexport)
int bIsFirstTime = 1; 
void __attribute__((constructor)) swh_init(); // forward declaration
#else
#define _WINDOWS_DLL_EXPORT_ 
#endif

#line 10 "dj_flanger_1438.xml"

#include <sys/types.h>
#include "ladspa-util.h"

#define DELAY_TIME 0.005f

#define DJFLANGER_SYNC                 0
#define DJFLANGER_PERIOD               1
#define DJFLANGER_DEPTH                2
#define DJFLANGER_FEEDBACK             3
#define DJFLANGER_INPUT                4
#define DJFLANGER_OUTPUT               5

static LADSPA_Descriptor *djFlangerDescriptor = NULL;

typedef struct {
	LADSPA_Data *sync;
	LADSPA_Data *period;
	LADSPA_Data *depth;
	LADSPA_Data *feedback;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data *buffer;
	unsigned int buffer_mask;
	unsigned int buffer_pos;
	float        fs;
	unsigned int last_sync;
	float        x;
	float        y;
	LADSPA_Data run_adding_gain;
} DjFlanger;

_WINDOWS_DLL_EXPORT_
const LADSPA_Descriptor *ladspa_descriptor(unsigned long index) {

#ifdef WIN32
	if (bIsFirstTime) {
		swh_init();
		bIsFirstTime = 0;
	}
#endif
	switch (index) {
	case 0:
		return djFlangerDescriptor;
	default:
		return NULL;
	}
}

static void activateDjFlanger(LADSPA_Handle instance) {
	DjFlanger *plugin_data = (DjFlanger *)instance;
	LADSPA_Data *buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	unsigned int buffer_pos = plugin_data->buffer_pos;
	float fs = plugin_data->fs;
	unsigned int last_sync = plugin_data->last_sync;
	float x = plugin_data->x;
	float y = plugin_data->y;
#line 38 "dj_flanger_1438.xml"
	memset(buffer, 0, (buffer_mask + 1) * sizeof(LADSPA_Data));
	last_sync = 0;
	plugin_data->buffer = buffer;
	plugin_data->buffer_mask = buffer_mask;
	plugin_data->buffer_pos = buffer_pos;
	plugin_data->fs = fs;
	plugin_data->last_sync = last_sync;
	plugin_data->x = x;
	plugin_data->y = y;

}

static void cleanupDjFlanger(LADSPA_Handle instance) {
#line 103 "dj_flanger_1438.xml"
	DjFlanger *plugin_data = (DjFlanger *)instance;
	free(plugin_data->buffer);
	free(instance);
}

static void connectPortDjFlanger(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	DjFlanger *plugin;

	plugin = (DjFlanger *)instance;
	switch (port) {
	case DJFLANGER_SYNC:
		plugin->sync = data;
		break;
	case DJFLANGER_PERIOD:
		plugin->period = data;
		break;
	case DJFLANGER_DEPTH:
		plugin->depth = data;
		break;
	case DJFLANGER_FEEDBACK:
		plugin->feedback = data;
		break;
	case DJFLANGER_INPUT:
		plugin->input = data;
		break;
	case DJFLANGER_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateDjFlanger(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	DjFlanger *plugin_data = (DjFlanger *)malloc(sizeof(DjFlanger));
	LADSPA_Data *buffer = NULL;
	unsigned int buffer_mask;
	unsigned int buffer_pos;
	float fs;
	unsigned int last_sync;
	float x;
	float y;

#line 23 "dj_flanger_1438.xml"
	int buffer_size = 2048;

	fs = s_rate;
	while (buffer_size < fs * DELAY_TIME + 3.0f) {
	  buffer_size *= 2;
	}
	buffer = calloc(buffer_size, sizeof(LADSPA_Data));
	buffer_mask = buffer_size - 1;
	buffer_pos = 0;
	x = 0.5f;
	y = 0.0f;
	last_sync = 0;

	plugin_data->buffer = buffer;
	plugin_data->buffer_mask = buffer_mask;
	plugin_data->buffer_pos = buffer_pos;
	plugin_data->fs = fs;
	plugin_data->last_sync = last_sync;
	plugin_data->x = x;
	plugin_data->y = y;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runDjFlanger(LADSPA_Handle instance, unsigned long sample_count) {
	DjFlanger *plugin_data = (DjFlanger *)instance;

	/* LFO sync (float value) */
	const LADSPA_Data sync = *(plugin_data->sync);

	/* LFO period (s) (float value) */
	const LADSPA_Data period = *(plugin_data->period);

	/* LFO depth (ms) (float value) */
	const LADSPA_Data depth = *(plugin_data->depth);

	/* Feedback (%) (float value) */
	const LADSPA_Data feedback = *(plugin_data->feedback);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	unsigned int buffer_pos = plugin_data->buffer_pos;
	float fs = plugin_data->fs;
	unsigned int last_sync = plugin_data->last_sync;
	float x = plugin_data->x;
	float y = plugin_data->y;

#line 43 "dj_flanger_1438.xml"
	unsigned long pos;
	const float omega = 6.2831852f / (period * fs);
	const float dr = 0.001f * fs * depth;
	float fb;
	float d;
	float dout, out;
	unsigned int dof;

	if (feedback > 99.0f) {
	  fb = 0.99f;
	} else if (feedback < -99.0f) {
	  fb = -0.99f;
	} else {
	  fb = feedback * 0.01f;
	}

	if (sync > 0) {
	  if (!last_sync) {
	    x = 0.5f;
	    y = 0.0f;
	  }
	  plugin_data->last_sync = 1;
	} else {
	  plugin_data->last_sync = 0;
	}

	for (pos = 0; pos < sample_count; pos++) {
	  /* Write input into delay line */
	  buffer[buffer_pos] = input[pos];

	  /* Calcuate delay */
	  d = (x + 0.5f) * dr;

	  dof = f_round(d);
	  //dout = buffer[(buffer_pos - f_round(d)) & buffer_mask];
	  dout = cube_interp(d - floor(d),
	                     buffer[(buffer_pos - dof - 3) & buffer_mask],
	                     buffer[(buffer_pos - dof - 2) & buffer_mask],
	                     buffer[(buffer_pos - dof - 1) & buffer_mask],
	                     buffer[(buffer_pos - dof) & buffer_mask]);

	  /* Write output */
	  out = (buffer[buffer_pos] + dout) * 0.5f;
	  buffer[buffer_pos] = input[pos] + out * fb;
	  buffer_write(output[pos], out);

	  /* Roll ringbuffer */
	  buffer_pos = (buffer_pos + 1) & buffer_mask;

	  /* Run LFO */
	  x -= omega * y;
	  y += omega * x;
	}

	plugin_data->x = x;
	plugin_data->y = y;
	plugin_data->buffer_pos = buffer_pos;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainDjFlanger(LADSPA_Handle instance, LADSPA_Data gain) {
	((DjFlanger *)instance)->run_adding_gain = gain;
}

static void runAddingDjFlanger(LADSPA_Handle instance, unsigned long sample_count) {
	DjFlanger *plugin_data = (DjFlanger *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* LFO sync (float value) */
	const LADSPA_Data sync = *(plugin_data->sync);

	/* LFO period (s) (float value) */
	const LADSPA_Data period = *(plugin_data->period);

	/* LFO depth (ms) (float value) */
	const LADSPA_Data depth = *(plugin_data->depth);

	/* Feedback (%) (float value) */
	const LADSPA_Data feedback = *(plugin_data->feedback);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	unsigned int buffer_pos = plugin_data->buffer_pos;
	float fs = plugin_data->fs;
	unsigned int last_sync = plugin_data->last_sync;
	float x = plugin_data->x;
	float y = plugin_data->y;

#line 43 "dj_flanger_1438.xml"
	unsigned long pos;
	const float omega = 6.2831852f / (period * fs);
	const float dr = 0.001f * fs * depth;
	float fb;
	float d;
	float dout, out;
	unsigned int dof;

	if (feedback > 99.0f) {
	  fb = 0.99f;
	} else if (feedback < -99.0f) {
	  fb = -0.99f;
	} else {
	  fb = feedback * 0.01f;
	}

	if (sync > 0) {
	  if (!last_sync) {
	    x = 0.5f;
	    y = 0.0f;
	  }
	  plugin_data->last_sync = 1;
	} else {
	  plugin_data->last_sync = 0;
	}

	for (pos = 0; pos < sample_count; pos++) {
	  /* Write input into delay line */
	  buffer[buffer_pos] = input[pos];

	  /* Calcuate delay */
	  d = (x + 0.5f) * dr;

	  dof = f_round(d);
	  //dout = buffer[(buffer_pos - f_round(d)) & buffer_mask];
	  dout = cube_interp(d - floor(d),
	                     buffer[(buffer_pos - dof - 3) & buffer_mask],
	                     buffer[(buffer_pos - dof - 2) & buffer_mask],
	                     buffer[(buffer_pos - dof - 1) & buffer_mask],
	                     buffer[(buffer_pos - dof) & buffer_mask]);

	  /* Write output */
	  out = (buffer[buffer_pos] + dout) * 0.5f;
	  buffer[buffer_pos] = input[pos] + out * fb;
	  buffer_write(output[pos], out);

	  /* Roll ringbuffer */
	  buffer_pos = (buffer_pos + 1) & buffer_mask;

	  /* Run LFO */
	  x -= omega * y;
	  y += omega * x;
	}

	plugin_data->x = x;
	plugin_data->y = y;
	plugin_data->buffer_pos = buffer_pos;
}

void __attribute__((constructor)) swh_init() {
	char **port_names;
	LADSPA_PortDescriptor *port_descriptors;
	LADSPA_PortRangeHint *port_range_hints;

#ifdef ENABLE_NLS
#define D_(s) dgettext(PACKAGE, s)
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, PACKAGE_LOCALE_DIR);
#else
#define D_(s) (s)
#endif


	djFlangerDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (djFlangerDescriptor) {
		djFlangerDescriptor->UniqueID = 1438;
		djFlangerDescriptor->Label = "djFlanger";
		djFlangerDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		djFlangerDescriptor->Name =
		 D_("DJ flanger");
		djFlangerDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		djFlangerDescriptor->Copyright =
		 "GPL";
		djFlangerDescriptor->PortCount = 6;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(6,
		 sizeof(LADSPA_PortDescriptor));
		djFlangerDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(6,
		 sizeof(LADSPA_PortRangeHint));
		djFlangerDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(6, sizeof(char*));
		djFlangerDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for LFO sync */
		port_descriptors[DJFLANGER_SYNC] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DJFLANGER_SYNC] =
		 D_("LFO sync");
		port_range_hints[DJFLANGER_SYNC].HintDescriptor = 0;

		/* Parameters for LFO period (s) */
		port_descriptors[DJFLANGER_PERIOD] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DJFLANGER_PERIOD] =
		 D_("LFO period (s)");
		port_range_hints[DJFLANGER_PERIOD].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[DJFLANGER_PERIOD].LowerBound = 0.1;
		port_range_hints[DJFLANGER_PERIOD].UpperBound = 32.0;

		/* Parameters for LFO depth (ms) */
		port_descriptors[DJFLANGER_DEPTH] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DJFLANGER_DEPTH] =
		 D_("LFO depth (ms)");
		port_range_hints[DJFLANGER_DEPTH].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_HIGH;
		port_range_hints[DJFLANGER_DEPTH].LowerBound = 1;
		port_range_hints[DJFLANGER_DEPTH].UpperBound = 5;

		/* Parameters for Feedback (%) */
		port_descriptors[DJFLANGER_FEEDBACK] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DJFLANGER_FEEDBACK] =
		 D_("Feedback (%)");
		port_range_hints[DJFLANGER_FEEDBACK].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[DJFLANGER_FEEDBACK].LowerBound = -100;
		port_range_hints[DJFLANGER_FEEDBACK].UpperBound = 100;

		/* Parameters for Input */
		port_descriptors[DJFLANGER_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[DJFLANGER_INPUT] =
		 D_("Input");
		port_range_hints[DJFLANGER_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[DJFLANGER_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[DJFLANGER_OUTPUT] =
		 D_("Output");
		port_range_hints[DJFLANGER_OUTPUT].HintDescriptor = 0;

		djFlangerDescriptor->activate = activateDjFlanger;
		djFlangerDescriptor->cleanup = cleanupDjFlanger;
		djFlangerDescriptor->connect_port = connectPortDjFlanger;
		djFlangerDescriptor->deactivate = NULL;
		djFlangerDescriptor->instantiate = instantiateDjFlanger;
		djFlangerDescriptor->run = runDjFlanger;
		djFlangerDescriptor->run_adding = runAddingDjFlanger;
		djFlangerDescriptor->set_run_adding_gain = setRunAddingGainDjFlanger;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (djFlangerDescriptor) {
		free((LADSPA_PortDescriptor *)djFlangerDescriptor->PortDescriptors);
		free((char **)djFlangerDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)djFlangerDescriptor->PortRangeHints);
		free(djFlangerDescriptor);
	}

}
