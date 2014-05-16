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


#include "ladspa-util.h"

#define BASE_BUFFER 8 // Base buffer length (s)

#define FADDELAY_DELAY                 0
#define FADDELAY_FB_DB                 1
#define FADDELAY_INPUT                 2
#define FADDELAY_OUTPUT                3

static LADSPA_Descriptor *fadDelayDescriptor = NULL;

typedef struct {
	LADSPA_Data *delay;
	LADSPA_Data *fb_db;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data *buffer;
	unsigned long buffer_mask;
	unsigned long buffer_size;
	LADSPA_Data  last_in;
	int          last_phase;
	float        phase;
	long         sample_rate;
	LADSPA_Data run_adding_gain;
} FadDelay;

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
		return fadDelayDescriptor;
	default:
		return NULL;
	}
}

static void activateFadDelay(LADSPA_Handle instance) {
	FadDelay *plugin_data = (FadDelay *)instance;
	LADSPA_Data *buffer = plugin_data->buffer;
	unsigned long buffer_mask = plugin_data->buffer_mask;
	unsigned long buffer_size = plugin_data->buffer_size;
	LADSPA_Data last_in = plugin_data->last_in;
	int last_phase = plugin_data->last_phase;
	float phase = plugin_data->phase;
	long sample_rate = plugin_data->sample_rate;
	unsigned int i;

	for (i = 0; i < buffer_size; i++) {
	        buffer[i] = 0;
	}
	phase = 0;
	last_phase = 0;
	last_in = 0.0f;
	sample_rate = sample_rate;
	plugin_data->buffer = buffer;
	plugin_data->buffer_mask = buffer_mask;
	plugin_data->buffer_size = buffer_size;
	plugin_data->last_in = last_in;
	plugin_data->last_phase = last_phase;
	plugin_data->phase = phase;
	plugin_data->sample_rate = sample_rate;

}

static void cleanupFadDelay(LADSPA_Handle instance) {
	FadDelay *plugin_data = (FadDelay *)instance;
	free(plugin_data->buffer);
	free(instance);
}

static void connectPortFadDelay(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	FadDelay *plugin;

	plugin = (FadDelay *)instance;
	switch (port) {
	case FADDELAY_DELAY:
		plugin->delay = data;
		break;
	case FADDELAY_FB_DB:
		plugin->fb_db = data;
		break;
	case FADDELAY_INPUT:
		plugin->input = data;
		break;
	case FADDELAY_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateFadDelay(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	FadDelay *plugin_data = (FadDelay *)malloc(sizeof(FadDelay));
	LADSPA_Data *buffer = NULL;
	unsigned long buffer_mask;
	unsigned long buffer_size;
	LADSPA_Data last_in;
	int last_phase;
	float phase;
	long sample_rate;

	unsigned int min_bs;

	sample_rate = s_rate;
	min_bs = BASE_BUFFER * s_rate;
	for (buffer_size = 4096; buffer_size < min_bs;
	     buffer_size *= 2);
	buffer = calloc(buffer_size, sizeof(LADSPA_Data));
	buffer_mask = buffer_size - 1;
	phase = 0;
	last_phase = 0;
	last_in = 0.0f;

	plugin_data->buffer = buffer;
	plugin_data->buffer_mask = buffer_mask;
	plugin_data->buffer_size = buffer_size;
	plugin_data->last_in = last_in;
	plugin_data->last_phase = last_phase;
	plugin_data->phase = phase;
	plugin_data->sample_rate = sample_rate;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runFadDelay(LADSPA_Handle instance, unsigned long sample_count) {
	FadDelay *plugin_data = (FadDelay *)instance;

	/* Delay (seconds) (float value) */
	const LADSPA_Data delay = *(plugin_data->delay);

	/* Feedback (dB) (float value) */
	const LADSPA_Data fb_db = *(plugin_data->fb_db);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned long buffer_mask = plugin_data->buffer_mask;
	unsigned long buffer_size = plugin_data->buffer_size;
	LADSPA_Data last_in = plugin_data->last_in;
	int last_phase = plugin_data->last_phase;
	float phase = plugin_data->phase;
	long sample_rate = plugin_data->sample_rate;

	unsigned long int pos;
	float increment = (float)buffer_size / ((float)sample_rate *
	                                        f_max(fabs(delay), 0.01));
	float lin_int, lin_inc;
	int track;
	int fph;
	LADSPA_Data out;
	const float fb = DB_CO(fb_db);
	
	for (pos = 0; pos < sample_count; pos++) {
	        fph = f_round(floor(phase));
	        last_phase = fph;
	        lin_int = phase - (float)fph;
	        out = LIN_INTERP(lin_int, buffer[(fph+1) & buffer_mask],
	         buffer[(fph+2) & buffer_mask]);
	        phase += increment;
	        lin_inc = 1.0f / (floor(phase) - last_phase + 1);
	        lin_inc = lin_inc > 1.0f ? 1.0f : lin_inc;
	        lin_int = 0.0f;
	        for (track = last_phase; track < phase; track++) {
	                lin_int += lin_inc;
	                buffer[track % buffer_size] = out * fb +
	                 LIN_INTERP(lin_int, last_in, input[pos]);
	        }
	        last_in = input[pos];
	        buffer_write(output[pos], out);
	        if (phase >= buffer_size) {
	                phase -= buffer_size;
	        }
	}
	
	// Store current phase in instance
	plugin_data->phase = phase;
	plugin_data->last_phase = last_phase;
	plugin_data->last_in = last_in;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainFadDelay(LADSPA_Handle instance, LADSPA_Data gain) {
	((FadDelay *)instance)->run_adding_gain = gain;
}

static void runAddingFadDelay(LADSPA_Handle instance, unsigned long sample_count) {
	FadDelay *plugin_data = (FadDelay *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Delay (seconds) (float value) */
	const LADSPA_Data delay = *(plugin_data->delay);

	/* Feedback (dB) (float value) */
	const LADSPA_Data fb_db = *(plugin_data->fb_db);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned long buffer_mask = plugin_data->buffer_mask;
	unsigned long buffer_size = plugin_data->buffer_size;
	LADSPA_Data last_in = plugin_data->last_in;
	int last_phase = plugin_data->last_phase;
	float phase = plugin_data->phase;
	long sample_rate = plugin_data->sample_rate;

	unsigned long int pos;
	float increment = (float)buffer_size / ((float)sample_rate *
	                                        f_max(fabs(delay), 0.01));
	float lin_int, lin_inc;
	int track;
	int fph;
	LADSPA_Data out;
	const float fb = DB_CO(fb_db);
	
	for (pos = 0; pos < sample_count; pos++) {
	        fph = f_round(floor(phase));
	        last_phase = fph;
	        lin_int = phase - (float)fph;
	        out = LIN_INTERP(lin_int, buffer[(fph+1) & buffer_mask],
	         buffer[(fph+2) & buffer_mask]);
	        phase += increment;
	        lin_inc = 1.0f / (floor(phase) - last_phase + 1);
	        lin_inc = lin_inc > 1.0f ? 1.0f : lin_inc;
	        lin_int = 0.0f;
	        for (track = last_phase; track < phase; track++) {
	                lin_int += lin_inc;
	                buffer[track % buffer_size] = out * fb +
	                 LIN_INTERP(lin_int, last_in, input[pos]);
	        }
	        last_in = input[pos];
	        buffer_write(output[pos], out);
	        if (phase >= buffer_size) {
	                phase -= buffer_size;
	        }
	}
	
	// Store current phase in instance
	plugin_data->phase = phase;
	plugin_data->last_phase = last_phase;
	plugin_data->last_in = last_in;
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


	fadDelayDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (fadDelayDescriptor) {
		fadDelayDescriptor->UniqueID = 1192;
		fadDelayDescriptor->Label = "fadDelay";
		fadDelayDescriptor->Properties =
		 0;
		fadDelayDescriptor->Name =
		 D_("Fractionally Addressed Delay Line");
		fadDelayDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		fadDelayDescriptor->Copyright =
		 "GPL";
		fadDelayDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		fadDelayDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		fadDelayDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		fadDelayDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Delay (seconds) */
		port_descriptors[FADDELAY_DELAY] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[FADDELAY_DELAY] =
		 D_("Delay (seconds)");
		port_range_hints[FADDELAY_DELAY].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[FADDELAY_DELAY].LowerBound = 0.1;
		port_range_hints[FADDELAY_DELAY].UpperBound = 10;

		/* Parameters for Feedback (dB) */
		port_descriptors[FADDELAY_FB_DB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[FADDELAY_FB_DB] =
		 D_("Feedback (dB)");
		port_range_hints[FADDELAY_FB_DB].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[FADDELAY_FB_DB].LowerBound = -70;
		port_range_hints[FADDELAY_FB_DB].UpperBound = 0;

		/* Parameters for Input */
		port_descriptors[FADDELAY_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[FADDELAY_INPUT] =
		 D_("Input");
		port_range_hints[FADDELAY_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[FADDELAY_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[FADDELAY_OUTPUT] =
		 D_("Output");
		port_range_hints[FADDELAY_OUTPUT].HintDescriptor = 0;

		fadDelayDescriptor->activate = activateFadDelay;
		fadDelayDescriptor->cleanup = cleanupFadDelay;
		fadDelayDescriptor->connect_port = connectPortFadDelay;
		fadDelayDescriptor->deactivate = NULL;
		fadDelayDescriptor->instantiate = instantiateFadDelay;
		fadDelayDescriptor->run = runFadDelay;
		fadDelayDescriptor->run_adding = runAddingFadDelay;
		fadDelayDescriptor->set_run_adding_gain = setRunAddingGainFadDelay;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (fadDelayDescriptor) {
		free((LADSPA_PortDescriptor *)fadDelayDescriptor->PortDescriptors);
		free((char **)fadDelayDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)fadDelayDescriptor->PortRangeHints);
		free(fadDelayDescriptor);
	}

}
