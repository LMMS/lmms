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

#line 10 "sc1_1425.xml"

#include "util/db.h"
#include "util/rms.h"

#define A_TBL 256

#define SC1_ATTACK                     0
#define SC1_RELEASE                    1
#define SC1_THRESHOLD                  2
#define SC1_RATIO                      3
#define SC1_KNEE                       4
#define SC1_MAKEUP_GAIN                5
#define SC1_INPUT                      6
#define SC1_OUTPUT                     7

static LADSPA_Descriptor *sc1Descriptor = NULL;

typedef struct {
	LADSPA_Data *attack;
	LADSPA_Data *release;
	LADSPA_Data *threshold;
	LADSPA_Data *ratio;
	LADSPA_Data *knee;
	LADSPA_Data *makeup_gain;
	LADSPA_Data *input;
	LADSPA_Data *output;
	float        amp;
	float *      as;
	unsigned int count;
	float        env;
	float        gain;
	float        gain_t;
	rms_env *    rms;
	float        sum;
	LADSPA_Data run_adding_gain;
} Sc1;

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
		return sc1Descriptor;
	default:
		return NULL;
	}
}

static void cleanupSc1(LADSPA_Handle instance) {
#line 45 "sc1_1425.xml"
	Sc1 *plugin_data = (Sc1 *)instance;
	rms_env_free(plugin_data->rms);
	free(plugin_data->as);
	free(instance);
}

static void connectPortSc1(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Sc1 *plugin;

	plugin = (Sc1 *)instance;
	switch (port) {
	case SC1_ATTACK:
		plugin->attack = data;
		break;
	case SC1_RELEASE:
		plugin->release = data;
		break;
	case SC1_THRESHOLD:
		plugin->threshold = data;
		break;
	case SC1_RATIO:
		plugin->ratio = data;
		break;
	case SC1_KNEE:
		plugin->knee = data;
		break;
	case SC1_MAKEUP_GAIN:
		plugin->makeup_gain = data;
		break;
	case SC1_INPUT:
		plugin->input = data;
		break;
	case SC1_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateSc1(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Sc1 *plugin_data = (Sc1 *)malloc(sizeof(Sc1));
	float amp;
	float *as = NULL;
	unsigned int count;
	float env;
	float gain;
	float gain_t;
	rms_env *rms = NULL;
	float sum;

#line 24 "sc1_1425.xml"
	unsigned int i;
	float sample_rate = (float)s_rate;

	rms = rms_env_new();
	sum = 0.0f;
	amp = 0.0f;
	gain = 0.0f;
	gain_t = 0.0f;
	env = 0.0f;
	count = 0;

	as = malloc(A_TBL * sizeof(float));
	as[0] = 1.0f;
	for (i=1; i<A_TBL; i++) {
	  as[i] = expf(-1.0f / (sample_rate * (float)i / (float)A_TBL));
	}

	db_init();

	plugin_data->amp = amp;
	plugin_data->as = as;
	plugin_data->count = count;
	plugin_data->env = env;
	plugin_data->gain = gain;
	plugin_data->gain_t = gain_t;
	plugin_data->rms = rms;
	plugin_data->sum = sum;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runSc1(LADSPA_Handle instance, unsigned long sample_count) {
	Sc1 *plugin_data = (Sc1 *)instance;

	/* Attack time (ms) (float value) */
	const LADSPA_Data attack = *(plugin_data->attack);

	/* Release time (ms) (float value) */
	const LADSPA_Data release = *(plugin_data->release);

	/* Threshold level (dB) (float value) */
	const LADSPA_Data threshold = *(plugin_data->threshold);

	/* Ratio (1:n) (float value) */
	const LADSPA_Data ratio = *(plugin_data->ratio);

	/* Knee radius (dB) (float value) */
	const LADSPA_Data knee = *(plugin_data->knee);

	/* Makeup gain (dB) (float value) */
	const LADSPA_Data makeup_gain = *(plugin_data->makeup_gain);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	float amp = plugin_data->amp;
	float * as = plugin_data->as;
	unsigned int count = plugin_data->count;
	float env = plugin_data->env;
	float gain = plugin_data->gain;
	float gain_t = plugin_data->gain_t;
	rms_env * rms = plugin_data->rms;
	float sum = plugin_data->sum;

#line 50 "sc1_1425.xml"
	unsigned long pos;

	const float ga = as[f_round(attack * 0.001f * (float)(A_TBL-1))];
	const float gr = as[f_round(release * 0.001f * (float)(A_TBL-1))];
	const float rs = (ratio - 1.0f) / ratio;
	const float mug = db2lin(makeup_gain);
	const float knee_min = db2lin(threshold - knee);
	const float knee_max = db2lin(threshold + knee);
	const float ef_a = ga * 0.25f;
	const float ef_ai = 1.0f - ef_a;

	for (pos = 0; pos < sample_count; pos++) {
	  sum += input[pos] * input[pos];

	  if (amp > env) {
	    env = env * ga + amp * (1.0f - ga);
	  } else {
	    env = env * gr + amp * (1.0f - gr);
	  }
	  if (count++ % 4 == 3) {
	    amp = rms_env_process(rms, sum * 0.25f);
	    sum = 0.0f;
	    if (env <= knee_min) {
	      gain_t = 1.0f;
	    } else if (env < knee_max) {
	      const float x = -(threshold - knee - lin2db(env)) / knee;
	      gain_t = db2lin(-knee * rs * x * x * 0.25f);
	    } else {
	      gain_t = db2lin((threshold - lin2db(env)) * rs);
	    }
	  }
	  gain = gain * ef_a + gain_t * ef_ai;
	  buffer_write(output[pos], input[pos] * gain * mug);
	}
	plugin_data->sum = sum;
	plugin_data->amp = amp;
	plugin_data->gain = gain;
	plugin_data->gain_t = gain_t;
	plugin_data->env = env;
	plugin_data->count = count;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainSc1(LADSPA_Handle instance, LADSPA_Data gain) {
	((Sc1 *)instance)->run_adding_gain = gain;
}

static void runAddingSc1(LADSPA_Handle instance, unsigned long sample_count) {
	Sc1 *plugin_data = (Sc1 *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Attack time (ms) (float value) */
	const LADSPA_Data attack = *(plugin_data->attack);

	/* Release time (ms) (float value) */
	const LADSPA_Data release = *(plugin_data->release);

	/* Threshold level (dB) (float value) */
	const LADSPA_Data threshold = *(plugin_data->threshold);

	/* Ratio (1:n) (float value) */
	const LADSPA_Data ratio = *(plugin_data->ratio);

	/* Knee radius (dB) (float value) */
	const LADSPA_Data knee = *(plugin_data->knee);

	/* Makeup gain (dB) (float value) */
	const LADSPA_Data makeup_gain = *(plugin_data->makeup_gain);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	float amp = plugin_data->amp;
	float * as = plugin_data->as;
	unsigned int count = plugin_data->count;
	float env = plugin_data->env;
	float gain = plugin_data->gain;
	float gain_t = plugin_data->gain_t;
	rms_env * rms = plugin_data->rms;
	float sum = plugin_data->sum;

#line 50 "sc1_1425.xml"
	unsigned long pos;

	const float ga = as[f_round(attack * 0.001f * (float)(A_TBL-1))];
	const float gr = as[f_round(release * 0.001f * (float)(A_TBL-1))];
	const float rs = (ratio - 1.0f) / ratio;
	const float mug = db2lin(makeup_gain);
	const float knee_min = db2lin(threshold - knee);
	const float knee_max = db2lin(threshold + knee);
	const float ef_a = ga * 0.25f;
	const float ef_ai = 1.0f - ef_a;

	for (pos = 0; pos < sample_count; pos++) {
	  sum += input[pos] * input[pos];

	  if (amp > env) {
	    env = env * ga + amp * (1.0f - ga);
	  } else {
	    env = env * gr + amp * (1.0f - gr);
	  }
	  if (count++ % 4 == 3) {
	    amp = rms_env_process(rms, sum * 0.25f);
	    sum = 0.0f;
	    if (env <= knee_min) {
	      gain_t = 1.0f;
	    } else if (env < knee_max) {
	      const float x = -(threshold - knee - lin2db(env)) / knee;
	      gain_t = db2lin(-knee * rs * x * x * 0.25f);
	    } else {
	      gain_t = db2lin((threshold - lin2db(env)) * rs);
	    }
	  }
	  gain = gain * ef_a + gain_t * ef_ai;
	  buffer_write(output[pos], input[pos] * gain * mug);
	}
	plugin_data->sum = sum;
	plugin_data->amp = amp;
	plugin_data->gain = gain;
	plugin_data->gain_t = gain_t;
	plugin_data->env = env;
	plugin_data->count = count;
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


	sc1Descriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (sc1Descriptor) {
		sc1Descriptor->UniqueID = 1425;
		sc1Descriptor->Label = "sc1";
		sc1Descriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		sc1Descriptor->Name =
		 D_("SC1");
		sc1Descriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		sc1Descriptor->Copyright =
		 "GPL";
		sc1Descriptor->PortCount = 8;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(8,
		 sizeof(LADSPA_PortDescriptor));
		sc1Descriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(8,
		 sizeof(LADSPA_PortRangeHint));
		sc1Descriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(8, sizeof(char*));
		sc1Descriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Attack time (ms) */
		port_descriptors[SC1_ATTACK] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC1_ATTACK] =
		 D_("Attack time (ms)");
		port_range_hints[SC1_ATTACK].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[SC1_ATTACK].LowerBound = 2;
		port_range_hints[SC1_ATTACK].UpperBound = 400;

		/* Parameters for Release time (ms) */
		port_descriptors[SC1_RELEASE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC1_RELEASE] =
		 D_("Release time (ms)");
		port_range_hints[SC1_RELEASE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[SC1_RELEASE].LowerBound = 2;
		port_range_hints[SC1_RELEASE].UpperBound = 800;

		/* Parameters for Threshold level (dB) */
		port_descriptors[SC1_THRESHOLD] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC1_THRESHOLD] =
		 D_("Threshold level (dB)");
		port_range_hints[SC1_THRESHOLD].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MAXIMUM;
		port_range_hints[SC1_THRESHOLD].LowerBound = -30;
		port_range_hints[SC1_THRESHOLD].UpperBound = 0;

		/* Parameters for Ratio (1:n) */
		port_descriptors[SC1_RATIO] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC1_RATIO] =
		 D_("Ratio (1:n)");
		port_range_hints[SC1_RATIO].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[SC1_RATIO].LowerBound = 1;
		port_range_hints[SC1_RATIO].UpperBound = 10;

		/* Parameters for Knee radius (dB) */
		port_descriptors[SC1_KNEE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC1_KNEE] =
		 D_("Knee radius (dB)");
		port_range_hints[SC1_KNEE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[SC1_KNEE].LowerBound = 1;
		port_range_hints[SC1_KNEE].UpperBound = 10;

		/* Parameters for Makeup gain (dB) */
		port_descriptors[SC1_MAKEUP_GAIN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC1_MAKEUP_GAIN] =
		 D_("Makeup gain (dB)");
		port_range_hints[SC1_MAKEUP_GAIN].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[SC1_MAKEUP_GAIN].LowerBound = 0;
		port_range_hints[SC1_MAKEUP_GAIN].UpperBound = +24;

		/* Parameters for Input */
		port_descriptors[SC1_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[SC1_INPUT] =
		 D_("Input");
		port_range_hints[SC1_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[SC1_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[SC1_OUTPUT] =
		 D_("Output");
		port_range_hints[SC1_OUTPUT].HintDescriptor = 0;

		sc1Descriptor->activate = NULL;
		sc1Descriptor->cleanup = cleanupSc1;
		sc1Descriptor->connect_port = connectPortSc1;
		sc1Descriptor->deactivate = NULL;
		sc1Descriptor->instantiate = instantiateSc1;
		sc1Descriptor->run = runSc1;
		sc1Descriptor->run_adding = runAddingSc1;
		sc1Descriptor->set_run_adding_gain = setRunAddingGainSc1;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (sc1Descriptor) {
		free((LADSPA_PortDescriptor *)sc1Descriptor->PortDescriptors);
		free((char **)sc1Descriptor->PortNames);
		free((LADSPA_PortRangeHint *)sc1Descriptor->PortRangeHints);
		free(sc1Descriptor);
	}

}
