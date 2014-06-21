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

#line 10 "sc3_1427.xml"

#include "util/db.h"
#include "util/rms.h"

#define A_TBL 256

#define SC3_ATTACK                     0
#define SC3_RELEASE                    1
#define SC3_THRESHOLD                  2
#define SC3_RATIO                      3
#define SC3_KNEE                       4
#define SC3_MAKEUP_GAIN                5
#define SC3_CHAIN_BAL                  6
#define SC3_SIDECHAIN                  7
#define SC3_LEFT_IN                    8
#define SC3_RIGHT_IN                   9
#define SC3_LEFT_OUT                   10
#define SC3_RIGHT_OUT                  11

static LADSPA_Descriptor *sc3Descriptor = NULL;

typedef struct {
	LADSPA_Data *attack;
	LADSPA_Data *release;
	LADSPA_Data *threshold;
	LADSPA_Data *ratio;
	LADSPA_Data *knee;
	LADSPA_Data *makeup_gain;
	LADSPA_Data *chain_bal;
	LADSPA_Data *sidechain;
	LADSPA_Data *left_in;
	LADSPA_Data *right_in;
	LADSPA_Data *left_out;
	LADSPA_Data *right_out;
	float        amp;
	float *      as;
	unsigned int count;
	float        env;
	float        gain;
	float        gain_t;
	rms_env *    rms;
	float        sum;
	LADSPA_Data run_adding_gain;
} Sc3;

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
		return sc3Descriptor;
	default:
		return NULL;
	}
}

static void cleanupSc3(LADSPA_Handle instance) {
#line 44 "sc3_1427.xml"
	Sc3 *plugin_data = (Sc3 *)instance;
	rms_env_free(plugin_data->rms);
	free(plugin_data->as);
	free(instance);
}

static void connectPortSc3(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Sc3 *plugin;

	plugin = (Sc3 *)instance;
	switch (port) {
	case SC3_ATTACK:
		plugin->attack = data;
		break;
	case SC3_RELEASE:
		plugin->release = data;
		break;
	case SC3_THRESHOLD:
		plugin->threshold = data;
		break;
	case SC3_RATIO:
		plugin->ratio = data;
		break;
	case SC3_KNEE:
		plugin->knee = data;
		break;
	case SC3_MAKEUP_GAIN:
		plugin->makeup_gain = data;
		break;
	case SC3_CHAIN_BAL:
		plugin->chain_bal = data;
		break;
	case SC3_SIDECHAIN:
		plugin->sidechain = data;
		break;
	case SC3_LEFT_IN:
		plugin->left_in = data;
		break;
	case SC3_RIGHT_IN:
		plugin->right_in = data;
		break;
	case SC3_LEFT_OUT:
		plugin->left_out = data;
		break;
	case SC3_RIGHT_OUT:
		plugin->right_out = data;
		break;
	}
}

static LADSPA_Handle instantiateSc3(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Sc3 *plugin_data = (Sc3 *)malloc(sizeof(Sc3));
	float amp;
	float *as = NULL;
	unsigned int count;
	float env;
	float gain;
	float gain_t;
	rms_env *rms = NULL;
	float sum;

#line 23 "sc3_1427.xml"
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

static void runSc3(LADSPA_Handle instance, unsigned long sample_count) {
	Sc3 *plugin_data = (Sc3 *)instance;

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

	/* Chain balance (float value) */
	const LADSPA_Data chain_bal = *(plugin_data->chain_bal);

	/* Sidechain (array of floats of length sample_count) */
	const LADSPA_Data * const sidechain = plugin_data->sidechain;

	/* Left input (array of floats of length sample_count) */
	const LADSPA_Data * const left_in = plugin_data->left_in;

	/* Right input (array of floats of length sample_count) */
	const LADSPA_Data * const right_in = plugin_data->right_in;

	/* Left output (array of floats of length sample_count) */
	LADSPA_Data * const left_out = plugin_data->left_out;

	/* Right output (array of floats of length sample_count) */
	LADSPA_Data * const right_out = plugin_data->right_out;
	float amp = plugin_data->amp;
	float * as = plugin_data->as;
	unsigned int count = plugin_data->count;
	float env = plugin_data->env;
	float gain = plugin_data->gain;
	float gain_t = plugin_data->gain_t;
	rms_env * rms = plugin_data->rms;
	float sum = plugin_data->sum;

#line 49 "sc3_1427.xml"
	unsigned long pos;

	const float ga = as[f_round(attack * 0.001f * (float)(A_TBL-1))];
	const float gr = as[f_round(release * 0.001f * (float)(A_TBL-1))];
	const float rs = (ratio - 1.0f) / ratio;
	const float mug = db2lin(makeup_gain);
	const float knee_min = db2lin(threshold - knee);
	const float knee_max = db2lin(threshold + knee);
	const float chain_bali = 1.0f - chain_bal;
	const float ef_a = ga * 0.25f;
	const float ef_ai = 1.0f - ef_a;

	for (pos = 0; pos < sample_count; pos++) {
	  const float lev_in = chain_bali * (left_in[pos] + right_in[pos]) * 0.5f
	                       + chain_bal * sidechain[pos];
	  sum += lev_in * lev_in;

	  if (amp > env) {
	    env = env * ga + amp * (1.0f - ga);
	  } else {
	    env = env * gr + amp * (1.0f - gr);
	  }
	  if (count++ % 4 == 3) {
	    amp = rms_env_process(rms, sum * 0.25f);
	    sum = 0.0f;
	    if (isnan(env)) {
	      // This can happen sometimes, but I dont know why
	      env = 0.0f;
	    } else if (env <= knee_min) {
	      gain_t = 1.0f;
	    } else if (env < knee_max) {
	      const float x = -(threshold - knee - lin2db(env)) / knee;
	      gain_t = db2lin(-knee * rs * x * x * 0.25f);
	    } else {
	      gain_t = db2lin((threshold - lin2db(env)) * rs);
	    }
	  }
	  gain = gain * ef_a + gain_t * ef_ai;
	  buffer_write(left_out[pos], left_in[pos] * gain * mug);
	  buffer_write(right_out[pos], right_in[pos] * gain * mug);
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

static void setRunAddingGainSc3(LADSPA_Handle instance, LADSPA_Data gain) {
	((Sc3 *)instance)->run_adding_gain = gain;
}

static void runAddingSc3(LADSPA_Handle instance, unsigned long sample_count) {
	Sc3 *plugin_data = (Sc3 *)instance;
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

	/* Chain balance (float value) */
	const LADSPA_Data chain_bal = *(plugin_data->chain_bal);

	/* Sidechain (array of floats of length sample_count) */
	const LADSPA_Data * const sidechain = plugin_data->sidechain;

	/* Left input (array of floats of length sample_count) */
	const LADSPA_Data * const left_in = plugin_data->left_in;

	/* Right input (array of floats of length sample_count) */
	const LADSPA_Data * const right_in = plugin_data->right_in;

	/* Left output (array of floats of length sample_count) */
	LADSPA_Data * const left_out = plugin_data->left_out;

	/* Right output (array of floats of length sample_count) */
	LADSPA_Data * const right_out = plugin_data->right_out;
	float amp = plugin_data->amp;
	float * as = plugin_data->as;
	unsigned int count = plugin_data->count;
	float env = plugin_data->env;
	float gain = plugin_data->gain;
	float gain_t = plugin_data->gain_t;
	rms_env * rms = plugin_data->rms;
	float sum = plugin_data->sum;

#line 49 "sc3_1427.xml"
	unsigned long pos;

	const float ga = as[f_round(attack * 0.001f * (float)(A_TBL-1))];
	const float gr = as[f_round(release * 0.001f * (float)(A_TBL-1))];
	const float rs = (ratio - 1.0f) / ratio;
	const float mug = db2lin(makeup_gain);
	const float knee_min = db2lin(threshold - knee);
	const float knee_max = db2lin(threshold + knee);
	const float chain_bali = 1.0f - chain_bal;
	const float ef_a = ga * 0.25f;
	const float ef_ai = 1.0f - ef_a;

	for (pos = 0; pos < sample_count; pos++) {
	  const float lev_in = chain_bali * (left_in[pos] + right_in[pos]) * 0.5f
	                       + chain_bal * sidechain[pos];
	  sum += lev_in * lev_in;

	  if (amp > env) {
	    env = env * ga + amp * (1.0f - ga);
	  } else {
	    env = env * gr + amp * (1.0f - gr);
	  }
	  if (count++ % 4 == 3) {
	    amp = rms_env_process(rms, sum * 0.25f);
	    sum = 0.0f;
	    if (isnan(env)) {
	      // This can happen sometimes, but I dont know why
	      env = 0.0f;
	    } else if (env <= knee_min) {
	      gain_t = 1.0f;
	    } else if (env < knee_max) {
	      const float x = -(threshold - knee - lin2db(env)) / knee;
	      gain_t = db2lin(-knee * rs * x * x * 0.25f);
	    } else {
	      gain_t = db2lin((threshold - lin2db(env)) * rs);
	    }
	  }
	  gain = gain * ef_a + gain_t * ef_ai;
	  buffer_write(left_out[pos], left_in[pos] * gain * mug);
	  buffer_write(right_out[pos], right_in[pos] * gain * mug);
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


	sc3Descriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (sc3Descriptor) {
		sc3Descriptor->UniqueID = 1427;
		sc3Descriptor->Label = "sc3";
		sc3Descriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		sc3Descriptor->Name =
		 D_("SC3");
		sc3Descriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		sc3Descriptor->Copyright =
		 "GPL";
		sc3Descriptor->PortCount = 12;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(12,
		 sizeof(LADSPA_PortDescriptor));
		sc3Descriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(12,
		 sizeof(LADSPA_PortRangeHint));
		sc3Descriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(12, sizeof(char*));
		sc3Descriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Attack time (ms) */
		port_descriptors[SC3_ATTACK] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC3_ATTACK] =
		 D_("Attack time (ms)");
		port_range_hints[SC3_ATTACK].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[SC3_ATTACK].LowerBound = 2;
		port_range_hints[SC3_ATTACK].UpperBound = 400;

		/* Parameters for Release time (ms) */
		port_descriptors[SC3_RELEASE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC3_RELEASE] =
		 D_("Release time (ms)");
		port_range_hints[SC3_RELEASE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[SC3_RELEASE].LowerBound = 2;
		port_range_hints[SC3_RELEASE].UpperBound = 800;

		/* Parameters for Threshold level (dB) */
		port_descriptors[SC3_THRESHOLD] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC3_THRESHOLD] =
		 D_("Threshold level (dB)");
		port_range_hints[SC3_THRESHOLD].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MAXIMUM;
		port_range_hints[SC3_THRESHOLD].LowerBound = -30;
		port_range_hints[SC3_THRESHOLD].UpperBound = 0;

		/* Parameters for Ratio (1:n) */
		port_descriptors[SC3_RATIO] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC3_RATIO] =
		 D_("Ratio (1:n)");
		port_range_hints[SC3_RATIO].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[SC3_RATIO].LowerBound = 1;
		port_range_hints[SC3_RATIO].UpperBound = 10;

		/* Parameters for Knee radius (dB) */
		port_descriptors[SC3_KNEE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC3_KNEE] =
		 D_("Knee radius (dB)");
		port_range_hints[SC3_KNEE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[SC3_KNEE].LowerBound = 1;
		port_range_hints[SC3_KNEE].UpperBound = 10;

		/* Parameters for Makeup gain (dB) */
		port_descriptors[SC3_MAKEUP_GAIN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC3_MAKEUP_GAIN] =
		 D_("Makeup gain (dB)");
		port_range_hints[SC3_MAKEUP_GAIN].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[SC3_MAKEUP_GAIN].LowerBound = 0;
		port_range_hints[SC3_MAKEUP_GAIN].UpperBound = +24;

		/* Parameters for Chain balance */
		port_descriptors[SC3_CHAIN_BAL] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC3_CHAIN_BAL] =
		 D_("Chain balance");
		port_range_hints[SC3_CHAIN_BAL].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[SC3_CHAIN_BAL].LowerBound = 0;
		port_range_hints[SC3_CHAIN_BAL].UpperBound = 1;

		/* Parameters for Sidechain */
		port_descriptors[SC3_SIDECHAIN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[SC3_SIDECHAIN] =
		 D_("Sidechain");
		port_range_hints[SC3_SIDECHAIN].HintDescriptor = 0;

		/* Parameters for Left input */
		port_descriptors[SC3_LEFT_IN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[SC3_LEFT_IN] =
		 D_("Left input");
		port_range_hints[SC3_LEFT_IN].HintDescriptor = 0;

		/* Parameters for Right input */
		port_descriptors[SC3_RIGHT_IN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[SC3_RIGHT_IN] =
		 D_("Right input");
		port_range_hints[SC3_RIGHT_IN].HintDescriptor = 0;

		/* Parameters for Left output */
		port_descriptors[SC3_LEFT_OUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[SC3_LEFT_OUT] =
		 D_("Left output");
		port_range_hints[SC3_LEFT_OUT].HintDescriptor = 0;

		/* Parameters for Right output */
		port_descriptors[SC3_RIGHT_OUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[SC3_RIGHT_OUT] =
		 D_("Right output");
		port_range_hints[SC3_RIGHT_OUT].HintDescriptor = 0;

		sc3Descriptor->activate = NULL;
		sc3Descriptor->cleanup = cleanupSc3;
		sc3Descriptor->connect_port = connectPortSc3;
		sc3Descriptor->deactivate = NULL;
		sc3Descriptor->instantiate = instantiateSc3;
		sc3Descriptor->run = runSc3;
		sc3Descriptor->run_adding = runAddingSc3;
		sc3Descriptor->set_run_adding_gain = setRunAddingGainSc3;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (sc3Descriptor) {
		free((LADSPA_PortDescriptor *)sc3Descriptor->PortDescriptors);
		free((char **)sc3Descriptor->PortNames);
		free((LADSPA_PortRangeHint *)sc3Descriptor->PortRangeHints);
		free(sc3Descriptor);
	}

}
