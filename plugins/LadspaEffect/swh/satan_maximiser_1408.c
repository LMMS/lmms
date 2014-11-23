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

#line 10 "satan_maximiser_1408.xml"

#include <math.h>
#include "ladspa-util.h"

#define BUFFER_SIZE 16
#define BUFFER_MASK 15

#define SATANMAXIMISER_ENV_TIME_P      0
#define SATANMAXIMISER_KNEE_POINT      1
#define SATANMAXIMISER_INPUT           2
#define SATANMAXIMISER_OUTPUT          3

static LADSPA_Descriptor *satanMaximiserDescriptor = NULL;

typedef struct {
	LADSPA_Data *env_time_p;
	LADSPA_Data *knee_point;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data *buffer;
	unsigned int buffer_pos;
	float        env;
	LADSPA_Data run_adding_gain;
} SatanMaximiser;

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
		return satanMaximiserDescriptor;
	default:
		return NULL;
	}
}

static void activateSatanMaximiser(LADSPA_Handle instance) {
	SatanMaximiser *plugin_data = (SatanMaximiser *)instance;
	LADSPA_Data *buffer = plugin_data->buffer;
	unsigned int buffer_pos = plugin_data->buffer_pos;
	float env = plugin_data->env;
#line 33 "satan_maximiser_1408.xml"
	env = 0.0f;
	memset(buffer, 0, sizeof(LADSPA_Data) * BUFFER_SIZE);
	buffer_pos = 0;
	plugin_data->buffer = buffer;
	plugin_data->buffer_pos = buffer_pos;
	plugin_data->env = env;

}

static void cleanupSatanMaximiser(LADSPA_Handle instance) {
#line 39 "satan_maximiser_1408.xml"
	SatanMaximiser *plugin_data = (SatanMaximiser *)instance;
	free(plugin_data->buffer);
	free(instance);
}

static void connectPortSatanMaximiser(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	SatanMaximiser *plugin;

	plugin = (SatanMaximiser *)instance;
	switch (port) {
	case SATANMAXIMISER_ENV_TIME_P:
		plugin->env_time_p = data;
		break;
	case SATANMAXIMISER_KNEE_POINT:
		plugin->knee_point = data;
		break;
	case SATANMAXIMISER_INPUT:
		plugin->input = data;
		break;
	case SATANMAXIMISER_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateSatanMaximiser(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	SatanMaximiser *plugin_data = (SatanMaximiser *)malloc(sizeof(SatanMaximiser));
	LADSPA_Data *buffer = NULL;
	unsigned int buffer_pos;
	float env;

#line 27 "satan_maximiser_1408.xml"
	env = 0.0f;
	buffer = malloc(sizeof(LADSPA_Data) * BUFFER_SIZE);
	buffer_pos = 0;

	plugin_data->buffer = buffer;
	plugin_data->buffer_pos = buffer_pos;
	plugin_data->env = env;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runSatanMaximiser(LADSPA_Handle instance, unsigned long sample_count) {
	SatanMaximiser *plugin_data = (SatanMaximiser *)instance;

	/* Decay time (samples) (float value) */
	const LADSPA_Data env_time_p = *(plugin_data->env_time_p);

	/* Knee point (dB) (float value) */
	const LADSPA_Data knee_point = *(plugin_data->knee_point);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_pos = plugin_data->buffer_pos;
	float env = plugin_data->env;

#line 43 "satan_maximiser_1408.xml"
	unsigned long pos;
	int delay;
	float env_tr, env_sc, knee;
	float env_time = env_time_p;

	if (env_time < 2.0f) {
	  env_time = 2.0f;
	}
	knee = DB_CO(knee_point);
	delay = f_round(env_time * 0.5f);
	env_tr = 1.0f / env_time;

	for (pos = 0; pos < sample_count; pos++) {
	  if (fabs(input[pos]) > env) {
	    env = fabs(input[pos]);
	  } else {
	    env = fabs(input[pos]) * env_tr + env * (1.0f - env_tr);
	  }
	  if (env <= knee) {
	    env_sc = 1.0f / knee;
	  } else {
	    env_sc = 1.0f / env;
	  }
	  buffer[buffer_pos] = input[pos];
	  buffer_write(output[pos], buffer[(buffer_pos - delay) & BUFFER_MASK] * env_sc);
	  buffer_pos = (buffer_pos + 1) & BUFFER_MASK;
	}

	plugin_data->env = env;
	plugin_data->buffer_pos = buffer_pos;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainSatanMaximiser(LADSPA_Handle instance, LADSPA_Data gain) {
	((SatanMaximiser *)instance)->run_adding_gain = gain;
}

static void runAddingSatanMaximiser(LADSPA_Handle instance, unsigned long sample_count) {
	SatanMaximiser *plugin_data = (SatanMaximiser *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Decay time (samples) (float value) */
	const LADSPA_Data env_time_p = *(plugin_data->env_time_p);

	/* Knee point (dB) (float value) */
	const LADSPA_Data knee_point = *(plugin_data->knee_point);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_pos = plugin_data->buffer_pos;
	float env = plugin_data->env;

#line 43 "satan_maximiser_1408.xml"
	unsigned long pos;
	int delay;
	float env_tr, env_sc, knee;
	float env_time = env_time_p;

	if (env_time < 2.0f) {
	  env_time = 2.0f;
	}
	knee = DB_CO(knee_point);
	delay = f_round(env_time * 0.5f);
	env_tr = 1.0f / env_time;

	for (pos = 0; pos < sample_count; pos++) {
	  if (fabs(input[pos]) > env) {
	    env = fabs(input[pos]);
	  } else {
	    env = fabs(input[pos]) * env_tr + env * (1.0f - env_tr);
	  }
	  if (env <= knee) {
	    env_sc = 1.0f / knee;
	  } else {
	    env_sc = 1.0f / env;
	  }
	  buffer[buffer_pos] = input[pos];
	  buffer_write(output[pos], buffer[(buffer_pos - delay) & BUFFER_MASK] * env_sc);
	  buffer_pos = (buffer_pos + 1) & BUFFER_MASK;
	}

	plugin_data->env = env;
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


	satanMaximiserDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (satanMaximiserDescriptor) {
		satanMaximiserDescriptor->UniqueID = 1408;
		satanMaximiserDescriptor->Label = "satanMaximiser";
		satanMaximiserDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		satanMaximiserDescriptor->Name =
		 D_("Barry's Satan Maximiser");
		satanMaximiserDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		satanMaximiserDescriptor->Copyright =
		 "GPL";
		satanMaximiserDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		satanMaximiserDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		satanMaximiserDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		satanMaximiserDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Decay time (samples) */
		port_descriptors[SATANMAXIMISER_ENV_TIME_P] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SATANMAXIMISER_ENV_TIME_P] =
		 D_("Decay time (samples)");
		port_range_hints[SATANMAXIMISER_ENV_TIME_P].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MAXIMUM;
		port_range_hints[SATANMAXIMISER_ENV_TIME_P].LowerBound = 2;
		port_range_hints[SATANMAXIMISER_ENV_TIME_P].UpperBound = 30;

		/* Parameters for Knee point (dB) */
		port_descriptors[SATANMAXIMISER_KNEE_POINT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SATANMAXIMISER_KNEE_POINT] =
		 D_("Knee point (dB)");
		port_range_hints[SATANMAXIMISER_KNEE_POINT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[SATANMAXIMISER_KNEE_POINT].LowerBound = -90;
		port_range_hints[SATANMAXIMISER_KNEE_POINT].UpperBound = 0;

		/* Parameters for Input */
		port_descriptors[SATANMAXIMISER_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[SATANMAXIMISER_INPUT] =
		 D_("Input");
		port_range_hints[SATANMAXIMISER_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[SATANMAXIMISER_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[SATANMAXIMISER_OUTPUT] =
		 D_("Output");
		port_range_hints[SATANMAXIMISER_OUTPUT].HintDescriptor = 0;

		satanMaximiserDescriptor->activate = activateSatanMaximiser;
		satanMaximiserDescriptor->cleanup = cleanupSatanMaximiser;
		satanMaximiserDescriptor->connect_port = connectPortSatanMaximiser;
		satanMaximiserDescriptor->deactivate = NULL;
		satanMaximiserDescriptor->instantiate = instantiateSatanMaximiser;
		satanMaximiserDescriptor->run = runSatanMaximiser;
		satanMaximiserDescriptor->run_adding = runAddingSatanMaximiser;
		satanMaximiserDescriptor->set_run_adding_gain = setRunAddingGainSatanMaximiser;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (satanMaximiserDescriptor) {
		free((LADSPA_PortDescriptor *)satanMaximiserDescriptor->PortDescriptors);
		free((char **)satanMaximiserDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)satanMaximiserDescriptor->PortRangeHints);
		free(satanMaximiserDescriptor);
	}

}
