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

#define LOG001 -6.9077552789f

#define DECAY_IN                       0
#define DECAY_OUT                      1
#define DECAY_DECAY_TIME               2

static LADSPA_Descriptor *decayDescriptor = NULL;

typedef struct {
	LADSPA_Data *in;
	LADSPA_Data *out;
	LADSPA_Data *decay_time;
	LADSPA_Data  b;
	char         first_time;
	LADSPA_Data  last_decay_time;
	LADSPA_Data  sample_rate;
	LADSPA_Data  y;
	LADSPA_Data run_adding_gain;
} Decay;

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
		return decayDescriptor;
	default:
		return NULL;
	}
}

static void activateDecay(LADSPA_Handle instance) {
	Decay *plugin_data = (Decay *)instance;
	LADSPA_Data b = plugin_data->b;
	char first_time = plugin_data->first_time;
	LADSPA_Data last_decay_time = plugin_data->last_decay_time;
	LADSPA_Data sample_rate = plugin_data->sample_rate;
	LADSPA_Data y = plugin_data->y;
	b = 0.f;
	y = 0.f;
	last_decay_time = 0.f;
	first_time = 0;
	plugin_data->b = b;
	plugin_data->first_time = first_time;
	plugin_data->last_decay_time = last_decay_time;
	plugin_data->sample_rate = sample_rate;
	plugin_data->y = y;

}

static void cleanupDecay(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortDecay(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Decay *plugin;

	plugin = (Decay *)instance;
	switch (port) {
	case DECAY_IN:
		plugin->in = data;
		break;
	case DECAY_OUT:
		plugin->out = data;
		break;
	case DECAY_DECAY_TIME:
		plugin->decay_time = data;
		break;
	}
}

static LADSPA_Handle instantiateDecay(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Decay *plugin_data = (Decay *)malloc(sizeof(Decay));
	LADSPA_Data b = 0;
	char first_time = 0;
	LADSPA_Data last_decay_time = 0;
	LADSPA_Data sample_rate = 0;
	LADSPA_Data y = 0;

	sample_rate = s_rate;

	plugin_data->b = b;
	plugin_data->first_time = first_time;
	plugin_data->last_decay_time = last_decay_time;
	plugin_data->sample_rate = sample_rate;
	plugin_data->y = y;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runDecay(LADSPA_Handle instance, unsigned long sample_count) {
	Decay *plugin_data = (Decay *)instance;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const in = plugin_data->in;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const out = plugin_data->out;

	/* Decay Time (s) (float value) */
	const LADSPA_Data decay_time = *(plugin_data->decay_time);
	LADSPA_Data b = plugin_data->b;
	char first_time = plugin_data->first_time;
	LADSPA_Data last_decay_time = plugin_data->last_decay_time;
	LADSPA_Data sample_rate = plugin_data->sample_rate;
	LADSPA_Data y = plugin_data->y;

	unsigned int i;

	if (first_time) {
	  plugin_data->last_decay_time = decay_time;
	  plugin_data->b = decay_time == 0.f ? 0.f : exp (LOG001 / (decay_time * sample_rate));
	  plugin_data->first_time = 0;
	}

	if (decay_time == last_decay_time) {
	  if (b == 0.f)
	    for (i=0; i<sample_count; i++)
	      out[i] = y = in[i];
	  else
	    for (i=0; i<sample_count; i++)
	      out[i] = y = in[i] + b * y;
	} else {
	  LADSPA_Data b_slope;

	  plugin_data->b = decay_time == 0.f ? 0.f : exp (LOG001 / (decay_time * sample_rate));
	  b_slope = (plugin_data->b - b) / sample_count;

	  for (i=0; i<sample_count; i++) {
	    buffer_write(out[i], y = in[i] + b * y);
	    b += b_slope;
	  }

	  plugin_data->last_decay_time = decay_time;
	}
	
	plugin_data->y = y;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainDecay(LADSPA_Handle instance, LADSPA_Data gain) {
	((Decay *)instance)->run_adding_gain = gain;
}

static void runAddingDecay(LADSPA_Handle instance, unsigned long sample_count) {
	Decay *plugin_data = (Decay *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const in = plugin_data->in;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const out = plugin_data->out;

	/* Decay Time (s) (float value) */
	const LADSPA_Data decay_time = *(plugin_data->decay_time);
	LADSPA_Data b = plugin_data->b;
	char first_time = plugin_data->first_time;
	LADSPA_Data last_decay_time = plugin_data->last_decay_time;
	LADSPA_Data sample_rate = plugin_data->sample_rate;
	LADSPA_Data y = plugin_data->y;

	unsigned int i;

	if (first_time) {
	  plugin_data->last_decay_time = decay_time;
	  plugin_data->b = decay_time == 0.f ? 0.f : exp (LOG001 / (decay_time * sample_rate));
	  plugin_data->first_time = 0;
	}

	if (decay_time == last_decay_time) {
	  if (b == 0.f)
	    for (i=0; i<sample_count; i++)
	      out[i] = y = in[i];
	  else
	    for (i=0; i<sample_count; i++)
	      out[i] = y = in[i] + b * y;
	} else {
	  LADSPA_Data b_slope;

	  plugin_data->b = decay_time == 0.f ? 0.f : exp (LOG001 / (decay_time * sample_rate));
	  b_slope = (plugin_data->b - b) / sample_count;

	  for (i=0; i<sample_count; i++) {
	    buffer_write(out[i], y = in[i] + b * y);
	    b += b_slope;
	  }

	  plugin_data->last_decay_time = decay_time;
	}
	
	plugin_data->y = y;
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


	decayDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (decayDescriptor) {
		decayDescriptor->UniqueID = 1886;
		decayDescriptor->Label = "decay";
		decayDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		decayDescriptor->Name =
		 D_("Exponential signal decay");
		decayDescriptor->Maker =
		 "Andy Wingo <wingo at pobox dot com>";
		decayDescriptor->Copyright =
		 "GPL";
		decayDescriptor->PortCount = 3;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(3,
		 sizeof(LADSPA_PortDescriptor));
		decayDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(3,
		 sizeof(LADSPA_PortRangeHint));
		decayDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(3, sizeof(char*));
		decayDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Input */
		port_descriptors[DECAY_IN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[DECAY_IN] =
		 D_("Input");
		port_range_hints[DECAY_IN].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[DECAY_OUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[DECAY_OUT] =
		 D_("Output");
		port_range_hints[DECAY_OUT].HintDescriptor = 0;

		/* Parameters for Decay Time (s) */
		port_descriptors[DECAY_DECAY_TIME] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DECAY_DECAY_TIME] =
		 D_("Decay Time (s)");
		port_range_hints[DECAY_DECAY_TIME].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[DECAY_DECAY_TIME].LowerBound = 0;

		decayDescriptor->activate = activateDecay;
		decayDescriptor->cleanup = cleanupDecay;
		decayDescriptor->connect_port = connectPortDecay;
		decayDescriptor->deactivate = NULL;
		decayDescriptor->instantiate = instantiateDecay;
		decayDescriptor->run = runDecay;
		decayDescriptor->run_adding = runAddingDecay;
		decayDescriptor->set_run_adding_gain = setRunAddingGainDecay;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (decayDescriptor) {
		free((LADSPA_PortDescriptor *)decayDescriptor->PortDescriptors);
		free((char **)decayDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)decayDescriptor->PortRangeHints);
		free(decayDescriptor);
	}

}
