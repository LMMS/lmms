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

#line 9 "delayorama_1402.xml"

#include <ladspa-util.h>

#define N_TAPS 128

typedef struct {
  unsigned int delay;
  float gain;
} tap;

#define DELAYORAMA_SEED                0
#define DELAYORAMA_GAIN                1
#define DELAYORAMA_FEEDBACK_PC         2
#define DELAYORAMA_TAP_COUNT           3
#define DELAYORAMA_FIRST_DELAY         4
#define DELAYORAMA_DELAY_RANGE         5
#define DELAYORAMA_DELAY_SCALE         6
#define DELAYORAMA_DELAY_RAND_PC       7
#define DELAYORAMA_GAIN_SCALE          8
#define DELAYORAMA_GAIN_RAND_PC        9
#define DELAYORAMA_WET                 10
#define DELAYORAMA_INPUT               11
#define DELAYORAMA_OUTPUT              12

static LADSPA_Descriptor *delayoramaDescriptor = NULL;

typedef struct {
	LADSPA_Data *seed;
	LADSPA_Data *gain;
	LADSPA_Data *feedback_pc;
	LADSPA_Data *tap_count;
	LADSPA_Data *first_delay;
	LADSPA_Data *delay_range;
	LADSPA_Data *delay_scale;
	LADSPA_Data *delay_rand_pc;
	LADSPA_Data *gain_scale;
	LADSPA_Data *gain_rand_pc;
	LADSPA_Data *wet;
	LADSPA_Data *input;
	LADSPA_Data *output;
	unsigned int active_set;
	LADSPA_Data *buffer;
	unsigned long buffer_pos;
	unsigned int buffer_size;
	float        last_a_rand;
	float        last_ampsc;
	float        last_d_rand;
	float        last_delaysc;
	unsigned int last_ntaps;
	LADSPA_Data  last_out;
	float        last_range;
	float        last_seed;
	float        last_start;
	unsigned int next_set;
	unsigned int sample_rate;
	tap **       taps;
	LADSPA_Data run_adding_gain;
} Delayorama;

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
		return delayoramaDescriptor;
	default:
		return NULL;
	}
}

static void activateDelayorama(LADSPA_Handle instance) {
	Delayorama *plugin_data = (Delayorama *)instance;
	unsigned int active_set = plugin_data->active_set;
	LADSPA_Data *buffer = plugin_data->buffer;
	unsigned long buffer_pos = plugin_data->buffer_pos;
	unsigned int buffer_size = plugin_data->buffer_size;
	float last_a_rand = plugin_data->last_a_rand;
	float last_ampsc = plugin_data->last_ampsc;
	float last_d_rand = plugin_data->last_d_rand;
	float last_delaysc = plugin_data->last_delaysc;
	unsigned int last_ntaps = plugin_data->last_ntaps;
	LADSPA_Data last_out = plugin_data->last_out;
	float last_range = plugin_data->last_range;
	float last_seed = plugin_data->last_seed;
	float last_start = plugin_data->last_start;
	unsigned int next_set = plugin_data->next_set;
	unsigned int sample_rate = plugin_data->sample_rate;
	tap **taps = plugin_data->taps;
#line 52 "delayorama_1402.xml"
	memset(buffer, 0, buffer_size * sizeof(LADSPA_Data));

	last_out = 0.0f;
	last_ampsc = 0.0f;
	last_delaysc = 0.0f;
	last_start = 0;
	last_range = 0;
	last_ntaps = 0;
	last_seed = 0;
	last_a_rand = 0;
	last_d_rand = 0;
	plugin_data->active_set = active_set;
	plugin_data->buffer = buffer;
	plugin_data->buffer_pos = buffer_pos;
	plugin_data->buffer_size = buffer_size;
	plugin_data->last_a_rand = last_a_rand;
	plugin_data->last_ampsc = last_ampsc;
	plugin_data->last_d_rand = last_d_rand;
	plugin_data->last_delaysc = last_delaysc;
	plugin_data->last_ntaps = last_ntaps;
	plugin_data->last_out = last_out;
	plugin_data->last_range = last_range;
	plugin_data->last_seed = last_seed;
	plugin_data->last_start = last_start;
	plugin_data->next_set = next_set;
	plugin_data->sample_rate = sample_rate;
	plugin_data->taps = taps;

}

static void cleanupDelayorama(LADSPA_Handle instance) {
#line 66 "delayorama_1402.xml"
	Delayorama *plugin_data = (Delayorama *)instance;
	free(plugin_data->taps[0]);
	free(plugin_data->taps[1]);
	free(plugin_data->taps);
	free(plugin_data->buffer);
	free(instance);
}

static void connectPortDelayorama(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Delayorama *plugin;

	plugin = (Delayorama *)instance;
	switch (port) {
	case DELAYORAMA_SEED:
		plugin->seed = data;
		break;
	case DELAYORAMA_GAIN:
		plugin->gain = data;
		break;
	case DELAYORAMA_FEEDBACK_PC:
		plugin->feedback_pc = data;
		break;
	case DELAYORAMA_TAP_COUNT:
		plugin->tap_count = data;
		break;
	case DELAYORAMA_FIRST_DELAY:
		plugin->first_delay = data;
		break;
	case DELAYORAMA_DELAY_RANGE:
		plugin->delay_range = data;
		break;
	case DELAYORAMA_DELAY_SCALE:
		plugin->delay_scale = data;
		break;
	case DELAYORAMA_DELAY_RAND_PC:
		plugin->delay_rand_pc = data;
		break;
	case DELAYORAMA_GAIN_SCALE:
		plugin->gain_scale = data;
		break;
	case DELAYORAMA_GAIN_RAND_PC:
		plugin->gain_rand_pc = data;
		break;
	case DELAYORAMA_WET:
		plugin->wet = data;
		break;
	case DELAYORAMA_INPUT:
		plugin->input = data;
		break;
	case DELAYORAMA_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateDelayorama(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Delayorama *plugin_data = (Delayorama *)malloc(sizeof(Delayorama));
	unsigned int active_set;
	LADSPA_Data *buffer = NULL;
	unsigned long buffer_pos;
	unsigned int buffer_size;
	float last_a_rand;
	float last_ampsc;
	float last_d_rand;
	float last_delaysc;
	unsigned int last_ntaps;
	LADSPA_Data last_out;
	float last_range;
	float last_seed;
	float last_start;
	unsigned int next_set;
	unsigned int sample_rate;
	tap **taps = NULL;

#line 25 "delayorama_1402.xml"
	sample_rate = s_rate;

	buffer_pos = 0;

	buffer_size = 6.0f * sample_rate;

	taps = malloc(2 * sizeof(tap *));
	taps[0] = calloc(N_TAPS, sizeof(tap));
	taps[1] = calloc(N_TAPS, sizeof(tap));
	active_set = 0;
	next_set = 1;

	buffer = calloc(buffer_size, sizeof(LADSPA_Data));

	last_out = 0.0f;

	last_ampsc = 0.0f;
	last_delaysc = 0.0f;
	last_start = 0;
	last_range = 0;
	last_ntaps = 0;
	last_seed = 0;
	last_a_rand = 0;
	last_d_rand = 0;

	plugin_data->active_set = active_set;
	plugin_data->buffer = buffer;
	plugin_data->buffer_pos = buffer_pos;
	plugin_data->buffer_size = buffer_size;
	plugin_data->last_a_rand = last_a_rand;
	plugin_data->last_ampsc = last_ampsc;
	plugin_data->last_d_rand = last_d_rand;
	plugin_data->last_delaysc = last_delaysc;
	plugin_data->last_ntaps = last_ntaps;
	plugin_data->last_out = last_out;
	plugin_data->last_range = last_range;
	plugin_data->last_seed = last_seed;
	plugin_data->last_start = last_start;
	plugin_data->next_set = next_set;
	plugin_data->sample_rate = sample_rate;
	plugin_data->taps = taps;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runDelayorama(LADSPA_Handle instance, unsigned long sample_count) {
	Delayorama *plugin_data = (Delayorama *)instance;

	/* Random seed (float value) */
	const LADSPA_Data seed = *(plugin_data->seed);

	/* Input gain (dB) (float value) */
	const LADSPA_Data gain = *(plugin_data->gain);

	/* Feedback (%) (float value) */
	const LADSPA_Data feedback_pc = *(plugin_data->feedback_pc);

	/* Number of taps (float value) */
	const LADSPA_Data tap_count = *(plugin_data->tap_count);

	/* First delay (s) (float value) */
	const LADSPA_Data first_delay = *(plugin_data->first_delay);

	/* Delay range (s) (float value) */
	const LADSPA_Data delay_range = *(plugin_data->delay_range);

	/* Delay change (float value) */
	const LADSPA_Data delay_scale = *(plugin_data->delay_scale);

	/* Delay random (%) (float value) */
	const LADSPA_Data delay_rand_pc = *(plugin_data->delay_rand_pc);

	/* Amplitude change (float value) */
	const LADSPA_Data gain_scale = *(plugin_data->gain_scale);

	/* Amplitude random (%) (float value) */
	const LADSPA_Data gain_rand_pc = *(plugin_data->gain_rand_pc);

	/* Dry/wet mix (float value) */
	const LADSPA_Data wet = *(plugin_data->wet);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	unsigned int active_set = plugin_data->active_set;
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned long buffer_pos = plugin_data->buffer_pos;
	unsigned int buffer_size = plugin_data->buffer_size;
	float last_a_rand = plugin_data->last_a_rand;
	float last_ampsc = plugin_data->last_ampsc;
	float last_d_rand = plugin_data->last_d_rand;
	float last_delaysc = plugin_data->last_delaysc;
	unsigned int last_ntaps = plugin_data->last_ntaps;
	LADSPA_Data last_out = plugin_data->last_out;
	float last_range = plugin_data->last_range;
	float last_seed = plugin_data->last_seed;
	float last_start = plugin_data->last_start;
	unsigned int next_set = plugin_data->next_set;
	unsigned int sample_rate = plugin_data->sample_rate;
	tap ** taps = plugin_data->taps;

#line 73 "delayorama_1402.xml"
	unsigned long pos;
	float coef = DB_CO(gain);
	unsigned int i;
	unsigned int recalc = 0;
	unsigned int ntaps = LIMIT(f_round(tap_count), 2, N_TAPS);
	float range = f_clamp(delay_range * sample_rate, 0.0f,
	                          (float)(buffer_size-1));
	LADSPA_Data out;
	float xfade = 0.0f;

	const float feedback = feedback_pc * 0.01f;
	const float gain_rand = gain_rand_pc * 0.01f;
	const float delay_rand = delay_rand_pc * 0.01f;


	if (ntaps != last_ntaps) {
	  recalc = 1;
	  plugin_data->last_ntaps = ntaps;
	}
	if (first_delay != last_start) {
	  recalc = 1;
	  plugin_data->last_start = first_delay;
	}
	if (range != last_range) {
	  recalc = 1;
	  plugin_data->last_range = range;
	}
	if (delay_scale != last_delaysc) {
	  recalc = 1;
	  plugin_data->last_delaysc = delay_scale;
	}
	if (gain_scale != last_ampsc) {
	  recalc = 1;
	  plugin_data->last_ampsc = gain_scale;
	}
	if (seed != last_seed) {
	  recalc = 1;
	  plugin_data->last_seed = seed;
	}
	if (gain_rand != last_a_rand) {
	  recalc = 1;
	  plugin_data->last_a_rand = gain_rand;
	}
	if (delay_rand != last_d_rand) {
	  recalc = 1;
	  plugin_data->last_d_rand = delay_rand;
	}

	if (recalc) {
	  float delay_base = first_delay * sample_rate;
	  float delay_fix;
	  float gain, delay, delay_sum;
	  float d_rand, g_rand;

	  srand(f_round(seed));
	  if (delay_base + range > buffer_size-1) {
	    delay_base = buffer_size - 1 - range;
	  }

	  if (gain_scale <= 1.0f) {
	    gain = 1.0f;
	  } else {
	    gain = 1.0f / pow(gain_scale, ntaps-1);
	  }

	  if (delay_scale == 1.0f) {
	          delay_fix = range / (ntaps - 1);
	  } else {
	          delay_fix = range * (delay_scale - 1.0f) / (pow(delay_scale, ntaps - 1) - 1.0f);
	  }
	  delay = 1.0f;
	  delay_sum = 0.0f;

	  for (i=0; i<ntaps; i++) {
	    g_rand = (1.0f-gain_rand) + (float)rand() / (float)RAND_MAX * 2.0f * gain_rand;
	    d_rand = (1.0f-delay_rand) + (float)rand() / (float)RAND_MAX * 2.0f * delay_rand;
	    taps[next_set][i].delay = LIMIT((unsigned int)(delay_base + delay_sum * delay_fix * d_rand), 0, buffer_size-1);
	    taps[next_set][i].gain = gain * g_rand;

	    delay_sum += delay;
	    delay *= delay_scale;
	    gain *= gain_scale;
	  }
	  for (; i<N_TAPS; i++) {
	    taps[next_set][i].delay = 0.0f;
	    taps[next_set][i].gain = 0.0f;
	  }
	}

	out = last_out;
	for (pos = 0; pos < sample_count; pos++) {
	  buffer[buffer_pos] = input[pos] * coef + (out * feedback);

	  out = 0.0f;
	  for (i=0; i<ntaps; i++) {
	    int p = buffer_pos - taps[active_set][i].delay;
	    if (p<0) p += buffer_size;
	    out += buffer[p] * taps[active_set][i].gain;
	  }

	  if (recalc) {
	    xfade += 1.0f / (float)sample_count;
	    out *= (1-xfade);
	    for (i=0; i<ntaps; i++) {
	      int p = buffer_pos - taps[next_set][i].delay;
	      if (p<0) p += buffer_size;
	      out += buffer[p] * taps[next_set][i].gain * xfade;
	    }
	  }

	  buffer_write(output[pos], LIN_INTERP(wet, input[pos], out));

	  if (++buffer_pos >= buffer_size) {
	    buffer_pos = 0;
	  }
	}

	if (recalc) {
	  plugin_data->active_set = next_set;
	  plugin_data->next_set = active_set;
	}

	plugin_data->buffer_pos = buffer_pos;
	plugin_data->last_out = out;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainDelayorama(LADSPA_Handle instance, LADSPA_Data gain) {
	((Delayorama *)instance)->run_adding_gain = gain;
}

static void runAddingDelayorama(LADSPA_Handle instance, unsigned long sample_count) {
	Delayorama *plugin_data = (Delayorama *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Random seed (float value) */
	const LADSPA_Data seed = *(plugin_data->seed);

	/* Input gain (dB) (float value) */
	const LADSPA_Data gain = *(plugin_data->gain);

	/* Feedback (%) (float value) */
	const LADSPA_Data feedback_pc = *(plugin_data->feedback_pc);

	/* Number of taps (float value) */
	const LADSPA_Data tap_count = *(plugin_data->tap_count);

	/* First delay (s) (float value) */
	const LADSPA_Data first_delay = *(plugin_data->first_delay);

	/* Delay range (s) (float value) */
	const LADSPA_Data delay_range = *(plugin_data->delay_range);

	/* Delay change (float value) */
	const LADSPA_Data delay_scale = *(plugin_data->delay_scale);

	/* Delay random (%) (float value) */
	const LADSPA_Data delay_rand_pc = *(plugin_data->delay_rand_pc);

	/* Amplitude change (float value) */
	const LADSPA_Data gain_scale = *(plugin_data->gain_scale);

	/* Amplitude random (%) (float value) */
	const LADSPA_Data gain_rand_pc = *(plugin_data->gain_rand_pc);

	/* Dry/wet mix (float value) */
	const LADSPA_Data wet = *(plugin_data->wet);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	unsigned int active_set = plugin_data->active_set;
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned long buffer_pos = plugin_data->buffer_pos;
	unsigned int buffer_size = plugin_data->buffer_size;
	float last_a_rand = plugin_data->last_a_rand;
	float last_ampsc = plugin_data->last_ampsc;
	float last_d_rand = plugin_data->last_d_rand;
	float last_delaysc = plugin_data->last_delaysc;
	unsigned int last_ntaps = plugin_data->last_ntaps;
	LADSPA_Data last_out = plugin_data->last_out;
	float last_range = plugin_data->last_range;
	float last_seed = plugin_data->last_seed;
	float last_start = plugin_data->last_start;
	unsigned int next_set = plugin_data->next_set;
	unsigned int sample_rate = plugin_data->sample_rate;
	tap ** taps = plugin_data->taps;

#line 73 "delayorama_1402.xml"
	unsigned long pos;
	float coef = DB_CO(gain);
	unsigned int i;
	unsigned int recalc = 0;
	unsigned int ntaps = LIMIT(f_round(tap_count), 2, N_TAPS);
	float range = f_clamp(delay_range * sample_rate, 0.0f,
	                          (float)(buffer_size-1));
	LADSPA_Data out;
	float xfade = 0.0f;

	const float feedback = feedback_pc * 0.01f;
	const float gain_rand = gain_rand_pc * 0.01f;
	const float delay_rand = delay_rand_pc * 0.01f;


	if (ntaps != last_ntaps) {
	  recalc = 1;
	  plugin_data->last_ntaps = ntaps;
	}
	if (first_delay != last_start) {
	  recalc = 1;
	  plugin_data->last_start = first_delay;
	}
	if (range != last_range) {
	  recalc = 1;
	  plugin_data->last_range = range;
	}
	if (delay_scale != last_delaysc) {
	  recalc = 1;
	  plugin_data->last_delaysc = delay_scale;
	}
	if (gain_scale != last_ampsc) {
	  recalc = 1;
	  plugin_data->last_ampsc = gain_scale;
	}
	if (seed != last_seed) {
	  recalc = 1;
	  plugin_data->last_seed = seed;
	}
	if (gain_rand != last_a_rand) {
	  recalc = 1;
	  plugin_data->last_a_rand = gain_rand;
	}
	if (delay_rand != last_d_rand) {
	  recalc = 1;
	  plugin_data->last_d_rand = delay_rand;
	}

	if (recalc) {
	  float delay_base = first_delay * sample_rate;
	  float delay_fix;
	  float gain, delay, delay_sum;
	  float d_rand, g_rand;

	  srand(f_round(seed));
	  if (delay_base + range > buffer_size-1) {
	    delay_base = buffer_size - 1 - range;
	  }

	  if (gain_scale <= 1.0f) {
	    gain = 1.0f;
	  } else {
	    gain = 1.0f / pow(gain_scale, ntaps-1);
	  }

	  if (delay_scale == 1.0f) {
	          delay_fix = range / (ntaps - 1);
	  } else {
	          delay_fix = range * (delay_scale - 1.0f) / (pow(delay_scale, ntaps - 1) - 1.0f);
	  }
	  delay = 1.0f;
	  delay_sum = 0.0f;

	  for (i=0; i<ntaps; i++) {
	    g_rand = (1.0f-gain_rand) + (float)rand() / (float)RAND_MAX * 2.0f * gain_rand;
	    d_rand = (1.0f-delay_rand) + (float)rand() / (float)RAND_MAX * 2.0f * delay_rand;
	    taps[next_set][i].delay = LIMIT((unsigned int)(delay_base + delay_sum * delay_fix * d_rand), 0, buffer_size-1);
	    taps[next_set][i].gain = gain * g_rand;

	    delay_sum += delay;
	    delay *= delay_scale;
	    gain *= gain_scale;
	  }
	  for (; i<N_TAPS; i++) {
	    taps[next_set][i].delay = 0.0f;
	    taps[next_set][i].gain = 0.0f;
	  }
	}

	out = last_out;
	for (pos = 0; pos < sample_count; pos++) {
	  buffer[buffer_pos] = input[pos] * coef + (out * feedback);

	  out = 0.0f;
	  for (i=0; i<ntaps; i++) {
	    int p = buffer_pos - taps[active_set][i].delay;
	    if (p<0) p += buffer_size;
	    out += buffer[p] * taps[active_set][i].gain;
	  }

	  if (recalc) {
	    xfade += 1.0f / (float)sample_count;
	    out *= (1-xfade);
	    for (i=0; i<ntaps; i++) {
	      int p = buffer_pos - taps[next_set][i].delay;
	      if (p<0) p += buffer_size;
	      out += buffer[p] * taps[next_set][i].gain * xfade;
	    }
	  }

	  buffer_write(output[pos], LIN_INTERP(wet, input[pos], out));

	  if (++buffer_pos >= buffer_size) {
	    buffer_pos = 0;
	  }
	}

	if (recalc) {
	  plugin_data->active_set = next_set;
	  plugin_data->next_set = active_set;
	}

	plugin_data->buffer_pos = buffer_pos;
	plugin_data->last_out = out;
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


	delayoramaDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (delayoramaDescriptor) {
		delayoramaDescriptor->UniqueID = 1402;
		delayoramaDescriptor->Label = "delayorama";
		delayoramaDescriptor->Properties =
		 0;
		delayoramaDescriptor->Name =
		 D_("Delayorama");
		delayoramaDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		delayoramaDescriptor->Copyright =
		 "GPL";
		delayoramaDescriptor->PortCount = 13;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(13,
		 sizeof(LADSPA_PortDescriptor));
		delayoramaDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(13,
		 sizeof(LADSPA_PortRangeHint));
		delayoramaDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(13, sizeof(char*));
		delayoramaDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Random seed */
		port_descriptors[DELAYORAMA_SEED] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DELAYORAMA_SEED] =
		 D_("Random seed");
		port_range_hints[DELAYORAMA_SEED].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_0;
		port_range_hints[DELAYORAMA_SEED].LowerBound = 0;
		port_range_hints[DELAYORAMA_SEED].UpperBound = 1000;

		/* Parameters for Input gain (dB) */
		port_descriptors[DELAYORAMA_GAIN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DELAYORAMA_GAIN] =
		 D_("Input gain (dB)");
		port_range_hints[DELAYORAMA_GAIN].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[DELAYORAMA_GAIN].LowerBound = -96;
		port_range_hints[DELAYORAMA_GAIN].UpperBound = +24;

		/* Parameters for Feedback (%) */
		port_descriptors[DELAYORAMA_FEEDBACK_PC] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DELAYORAMA_FEEDBACK_PC] =
		 D_("Feedback (%)");
		port_range_hints[DELAYORAMA_FEEDBACK_PC].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[DELAYORAMA_FEEDBACK_PC].LowerBound = 0;
		port_range_hints[DELAYORAMA_FEEDBACK_PC].UpperBound = 100;

		/* Parameters for Number of taps */
		port_descriptors[DELAYORAMA_TAP_COUNT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DELAYORAMA_TAP_COUNT] =
		 D_("Number of taps");
		port_range_hints[DELAYORAMA_TAP_COUNT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_MINIMUM;
		port_range_hints[DELAYORAMA_TAP_COUNT].LowerBound = 2;
		port_range_hints[DELAYORAMA_TAP_COUNT].UpperBound = N_TAPS;

		/* Parameters for First delay (s) */
		port_descriptors[DELAYORAMA_FIRST_DELAY] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DELAYORAMA_FIRST_DELAY] =
		 D_("First delay (s)");
		port_range_hints[DELAYORAMA_FIRST_DELAY].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[DELAYORAMA_FIRST_DELAY].LowerBound = 0;
		port_range_hints[DELAYORAMA_FIRST_DELAY].UpperBound = 5;

		/* Parameters for Delay range (s) */
		port_descriptors[DELAYORAMA_DELAY_RANGE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DELAYORAMA_DELAY_RANGE] =
		 D_("Delay range (s)");
		port_range_hints[DELAYORAMA_DELAY_RANGE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MAXIMUM;
		port_range_hints[DELAYORAMA_DELAY_RANGE].LowerBound = 0.0001;
		port_range_hints[DELAYORAMA_DELAY_RANGE].UpperBound = 6;

		/* Parameters for Delay change */
		port_descriptors[DELAYORAMA_DELAY_SCALE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DELAYORAMA_DELAY_SCALE] =
		 D_("Delay change");
		port_range_hints[DELAYORAMA_DELAY_SCALE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[DELAYORAMA_DELAY_SCALE].LowerBound = 0.2;
		port_range_hints[DELAYORAMA_DELAY_SCALE].UpperBound = 5;

		/* Parameters for Delay random (%) */
		port_descriptors[DELAYORAMA_DELAY_RAND_PC] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DELAYORAMA_DELAY_RAND_PC] =
		 D_("Delay random (%)");
		port_range_hints[DELAYORAMA_DELAY_RAND_PC].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[DELAYORAMA_DELAY_RAND_PC].LowerBound = 0;
		port_range_hints[DELAYORAMA_DELAY_RAND_PC].UpperBound = 100;

		/* Parameters for Amplitude change */
		port_descriptors[DELAYORAMA_GAIN_SCALE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DELAYORAMA_GAIN_SCALE] =
		 D_("Amplitude change");
		port_range_hints[DELAYORAMA_GAIN_SCALE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[DELAYORAMA_GAIN_SCALE].LowerBound = 0.2;
		port_range_hints[DELAYORAMA_GAIN_SCALE].UpperBound = 5;

		/* Parameters for Amplitude random (%) */
		port_descriptors[DELAYORAMA_GAIN_RAND_PC] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DELAYORAMA_GAIN_RAND_PC] =
		 D_("Amplitude random (%)");
		port_range_hints[DELAYORAMA_GAIN_RAND_PC].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[DELAYORAMA_GAIN_RAND_PC].LowerBound = 0;
		port_range_hints[DELAYORAMA_GAIN_RAND_PC].UpperBound = 100;

		/* Parameters for Dry/wet mix */
		port_descriptors[DELAYORAMA_WET] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DELAYORAMA_WET] =
		 D_("Dry/wet mix");
		port_range_hints[DELAYORAMA_WET].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[DELAYORAMA_WET].LowerBound = 0;
		port_range_hints[DELAYORAMA_WET].UpperBound = 1;

		/* Parameters for Input */
		port_descriptors[DELAYORAMA_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[DELAYORAMA_INPUT] =
		 D_("Input");
		port_range_hints[DELAYORAMA_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[DELAYORAMA_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[DELAYORAMA_OUTPUT] =
		 D_("Output");
		port_range_hints[DELAYORAMA_OUTPUT].HintDescriptor = 0;

		delayoramaDescriptor->activate = activateDelayorama;
		delayoramaDescriptor->cleanup = cleanupDelayorama;
		delayoramaDescriptor->connect_port = connectPortDelayorama;
		delayoramaDescriptor->deactivate = NULL;
		delayoramaDescriptor->instantiate = instantiateDelayorama;
		delayoramaDescriptor->run = runDelayorama;
		delayoramaDescriptor->run_adding = runAddingDelayorama;
		delayoramaDescriptor->set_run_adding_gain = setRunAddingGainDelayorama;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (delayoramaDescriptor) {
		free((LADSPA_PortDescriptor *)delayoramaDescriptor->PortDescriptors);
		free((char **)delayoramaDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)delayoramaDescriptor->PortRangeHints);
		free(delayoramaDescriptor);
	}

}
