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

#line 10 "sc2_1426.xml"

#include "util/db.h"
#include "util/rms.h"

#define A_TBL 256

#define SC2_ATTACK                     0
#define SC2_RELEASE                    1
#define SC2_THRESHOLD                  2
#define SC2_RATIO                      3
#define SC2_KNEE                       4
#define SC2_MAKEUP_GAIN                5
#define SC2_SIDECHAIN                  6
#define SC2_INPUT                      7
#define SC2_OUTPUT                     8

static LADSPA_Descriptor *sc2Descriptor = NULL;

typedef struct {
	LADSPA_Data *attack;
	LADSPA_Data *release;
	LADSPA_Data *threshold;
	LADSPA_Data *ratio;
	LADSPA_Data *knee;
	LADSPA_Data *makeup_gain;
	LADSPA_Data *sidechain;
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
} Sc2;

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
		return sc2Descriptor;
	default:
		return NULL;
	}
}

static void cleanupSc2(LADSPA_Handle instance) {
#line 44 "sc2_1426.xml"
	Sc2 *plugin_data = (Sc2 *)instance;
	rms_env_free(plugin_data->rms);
	free(plugin_data->as);
	free(instance);
}

static void connectPortSc2(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Sc2 *plugin;

	plugin = (Sc2 *)instance;
	switch (port) {
	case SC2_ATTACK:
		plugin->attack = data;
		break;
	case SC2_RELEASE:
		plugin->release = data;
		break;
	case SC2_THRESHOLD:
		plugin->threshold = data;
		break;
	case SC2_RATIO:
		plugin->ratio = data;
		break;
	case SC2_KNEE:
		plugin->knee = data;
		break;
	case SC2_MAKEUP_GAIN:
		plugin->makeup_gain = data;
		break;
	case SC2_SIDECHAIN:
		plugin->sidechain = data;
		break;
	case SC2_INPUT:
		plugin->input = data;
		break;
	case SC2_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateSc2(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Sc2 *plugin_data = (Sc2 *)malloc(sizeof(Sc2));
	float amp;
	float *as = NULL;
	unsigned int count;
	float env;
	float gain;
	float gain_t;
	rms_env *rms = NULL;
	float sum;

#line 23 "sc2_1426.xml"
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

static void runSc2(LADSPA_Handle instance, unsigned long sample_count) {
	Sc2 *plugin_data = (Sc2 *)instance;

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

	/* Sidechain (array of floats of length sample_count) */
	const LADSPA_Data * const sidechain = plugin_data->sidechain;

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

#line 49 "sc2_1426.xml"
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
	  sum += sidechain[pos] * sidechain[pos];

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

static void setRunAddingGainSc2(LADSPA_Handle instance, LADSPA_Data gain) {
	((Sc2 *)instance)->run_adding_gain = gain;
}

static void runAddingSc2(LADSPA_Handle instance, unsigned long sample_count) {
	Sc2 *plugin_data = (Sc2 *)instance;
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

	/* Sidechain (array of floats of length sample_count) */
	const LADSPA_Data * const sidechain = plugin_data->sidechain;

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

#line 49 "sc2_1426.xml"
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
	  sum += sidechain[pos] * sidechain[pos];

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


	sc2Descriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (sc2Descriptor) {
		sc2Descriptor->UniqueID = 1426;
		sc2Descriptor->Label = "sc2";
		sc2Descriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		sc2Descriptor->Name =
		 D_("SC2");
		sc2Descriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		sc2Descriptor->Copyright =
		 "GPL";
		sc2Descriptor->PortCount = 9;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(9,
		 sizeof(LADSPA_PortDescriptor));
		sc2Descriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(9,
		 sizeof(LADSPA_PortRangeHint));
		sc2Descriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(9, sizeof(char*));
		sc2Descriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Attack time (ms) */
		port_descriptors[SC2_ATTACK] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC2_ATTACK] =
		 D_("Attack time (ms)");
		port_range_hints[SC2_ATTACK].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[SC2_ATTACK].LowerBound = 2;
		port_range_hints[SC2_ATTACK].UpperBound = 400;

		/* Parameters for Release time (ms) */
		port_descriptors[SC2_RELEASE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC2_RELEASE] =
		 D_("Release time (ms)");
		port_range_hints[SC2_RELEASE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[SC2_RELEASE].LowerBound = 2;
		port_range_hints[SC2_RELEASE].UpperBound = 800;

		/* Parameters for Threshold level (dB) */
		port_descriptors[SC2_THRESHOLD] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC2_THRESHOLD] =
		 D_("Threshold level (dB)");
		port_range_hints[SC2_THRESHOLD].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MAXIMUM;
		port_range_hints[SC2_THRESHOLD].LowerBound = -30;
		port_range_hints[SC2_THRESHOLD].UpperBound = 0;

		/* Parameters for Ratio (1:n) */
		port_descriptors[SC2_RATIO] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC2_RATIO] =
		 D_("Ratio (1:n)");
		port_range_hints[SC2_RATIO].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[SC2_RATIO].LowerBound = 1;
		port_range_hints[SC2_RATIO].UpperBound = 10;

		/* Parameters for Knee radius (dB) */
		port_descriptors[SC2_KNEE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC2_KNEE] =
		 D_("Knee radius (dB)");
		port_range_hints[SC2_KNEE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[SC2_KNEE].LowerBound = 1;
		port_range_hints[SC2_KNEE].UpperBound = 10;

		/* Parameters for Makeup gain (dB) */
		port_descriptors[SC2_MAKEUP_GAIN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC2_MAKEUP_GAIN] =
		 D_("Makeup gain (dB)");
		port_range_hints[SC2_MAKEUP_GAIN].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[SC2_MAKEUP_GAIN].LowerBound = 0;
		port_range_hints[SC2_MAKEUP_GAIN].UpperBound = +24;

		/* Parameters for Sidechain */
		port_descriptors[SC2_SIDECHAIN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[SC2_SIDECHAIN] =
		 D_("Sidechain");
		port_range_hints[SC2_SIDECHAIN].HintDescriptor = 0;

		/* Parameters for Input */
		port_descriptors[SC2_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[SC2_INPUT] =
		 D_("Input");
		port_range_hints[SC2_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[SC2_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[SC2_OUTPUT] =
		 D_("Output");
		port_range_hints[SC2_OUTPUT].HintDescriptor = 0;

		sc2Descriptor->activate = NULL;
		sc2Descriptor->cleanup = cleanupSc2;
		sc2Descriptor->connect_port = connectPortSc2;
		sc2Descriptor->deactivate = NULL;
		sc2Descriptor->instantiate = instantiateSc2;
		sc2Descriptor->run = runSc2;
		sc2Descriptor->run_adding = runAddingSc2;
		sc2Descriptor->set_run_adding_gain = setRunAddingGainSc2;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (sc2Descriptor) {
		free((LADSPA_PortDescriptor *)sc2Descriptor->PortDescriptors);
		free((char **)sc2Descriptor->PortNames);
		free((LADSPA_PortRangeHint *)sc2Descriptor->PortRangeHints);
		free(sc2Descriptor);
	}

}
