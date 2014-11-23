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

#line 10 "latency_1914.xml"

#include "ladspa-util.h"

#define ARTIFICIALLATENCY_DELAY        0
#define ARTIFICIALLATENCY_INPUT        1
#define ARTIFICIALLATENCY_OUTPUT       2
#define ARTIFICIALLATENCY_LATENCY      3

static LADSPA_Descriptor *artificialLatencyDescriptor = NULL;

typedef struct {
	LADSPA_Data *delay;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data *latency;
	float        fs;
	LADSPA_Data run_adding_gain;
} ArtificialLatency;

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
		return artificialLatencyDescriptor;
	default:
		return NULL;
	}
}

static void cleanupArtificialLatency(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortArtificialLatency(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	ArtificialLatency *plugin;

	plugin = (ArtificialLatency *)instance;
	switch (port) {
	case ARTIFICIALLATENCY_DELAY:
		plugin->delay = data;
		break;
	case ARTIFICIALLATENCY_INPUT:
		plugin->input = data;
		break;
	case ARTIFICIALLATENCY_OUTPUT:
		plugin->output = data;
		break;
	case ARTIFICIALLATENCY_LATENCY:
		plugin->latency = data;
		break;
	}
}

static LADSPA_Handle instantiateArtificialLatency(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	ArtificialLatency *plugin_data = (ArtificialLatency *)malloc(sizeof(ArtificialLatency));
	float fs;

#line 21 "latency_1914.xml"
	fs = s_rate;

	plugin_data->fs = fs;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runArtificialLatency(LADSPA_Handle instance, unsigned long sample_count) {
	ArtificialLatency *plugin_data = (ArtificialLatency *)instance;

	/* Delay (ms) (float value) */
	const LADSPA_Data delay = *(plugin_data->delay);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	float fs = plugin_data->fs;

#line 25 "latency_1914.xml"
	unsigned long pos;
	const int delay_fr = f_round(delay * 0.001 * fs);

	if (input != output) {
	  for (pos = 0; pos < sample_count; pos++) {
	    buffer_write(output[pos], input[pos]);
	  }
	}
	*(plugin_data->latency) = (float)delay_fr;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainArtificialLatency(LADSPA_Handle instance, LADSPA_Data gain) {
	((ArtificialLatency *)instance)->run_adding_gain = gain;
}

static void runAddingArtificialLatency(LADSPA_Handle instance, unsigned long sample_count) {
	ArtificialLatency *plugin_data = (ArtificialLatency *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Delay (ms) (float value) */
	const LADSPA_Data delay = *(plugin_data->delay);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	float fs = plugin_data->fs;

#line 25 "latency_1914.xml"
	unsigned long pos;
	const int delay_fr = f_round(delay * 0.001 * fs);

	if (input != output) {
	  for (pos = 0; pos < sample_count; pos++) {
	    buffer_write(output[pos], input[pos]);
	  }
	}
	*(plugin_data->latency) = (float)delay_fr;
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


	artificialLatencyDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (artificialLatencyDescriptor) {
		artificialLatencyDescriptor->UniqueID = 1914;
		artificialLatencyDescriptor->Label = "artificialLatency";
		artificialLatencyDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		artificialLatencyDescriptor->Name =
		 D_("Artificial latency");
		artificialLatencyDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		artificialLatencyDescriptor->Copyright =
		 "GPL";
		artificialLatencyDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		artificialLatencyDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		artificialLatencyDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		artificialLatencyDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Delay (ms) */
		port_descriptors[ARTIFICIALLATENCY_DELAY] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[ARTIFICIALLATENCY_DELAY] =
		 D_("Delay (ms)");
		port_range_hints[ARTIFICIALLATENCY_DELAY].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[ARTIFICIALLATENCY_DELAY].LowerBound = 0;
		port_range_hints[ARTIFICIALLATENCY_DELAY].UpperBound = 10000;

		/* Parameters for Input */
		port_descriptors[ARTIFICIALLATENCY_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[ARTIFICIALLATENCY_INPUT] =
		 D_("Input");
		port_range_hints[ARTIFICIALLATENCY_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[ARTIFICIALLATENCY_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[ARTIFICIALLATENCY_OUTPUT] =
		 D_("Output");
		port_range_hints[ARTIFICIALLATENCY_OUTPUT].HintDescriptor = 0;

		/* Parameters for latency */
		port_descriptors[ARTIFICIALLATENCY_LATENCY] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
		port_names[ARTIFICIALLATENCY_LATENCY] =
		 D_("latency");
		port_range_hints[ARTIFICIALLATENCY_LATENCY].HintDescriptor = 0;

		artificialLatencyDescriptor->activate = NULL;
		artificialLatencyDescriptor->cleanup = cleanupArtificialLatency;
		artificialLatencyDescriptor->connect_port = connectPortArtificialLatency;
		artificialLatencyDescriptor->deactivate = NULL;
		artificialLatencyDescriptor->instantiate = instantiateArtificialLatency;
		artificialLatencyDescriptor->run = runArtificialLatency;
		artificialLatencyDescriptor->run_adding = runAddingArtificialLatency;
		artificialLatencyDescriptor->set_run_adding_gain = setRunAddingGainArtificialLatency;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (artificialLatencyDescriptor) {
		free((LADSPA_PortDescriptor *)artificialLatencyDescriptor->PortDescriptors);
		free((char **)artificialLatencyDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)artificialLatencyDescriptor->PortRangeHints);
		free(artificialLatencyDescriptor);
	}

}
