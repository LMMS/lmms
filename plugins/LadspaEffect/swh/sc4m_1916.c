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

#line 10 "sc4m_1916.xml"

#include "util/db.h"
#include "util/rms.h"

#define A_TBL 256

#define SC4M_RMS_PEAK                  0
#define SC4M_ATTACK                    1
#define SC4M_RELEASE                   2
#define SC4M_THRESHOLD                 3
#define SC4M_RATIO                     4
#define SC4M_KNEE                      5
#define SC4M_MAKEUP_GAIN               6
#define SC4M_AMPLITUDE                 7
#define SC4M_GAIN_RED                  8
#define SC4M_INPUT                     9
#define SC4M_OUTPUT                    10

static LADSPA_Descriptor *sc4mDescriptor = NULL;

typedef struct {
	LADSPA_Data *rms_peak;
	LADSPA_Data *attack;
	LADSPA_Data *release;
	LADSPA_Data *threshold;
	LADSPA_Data *ratio;
	LADSPA_Data *knee;
	LADSPA_Data *makeup_gain;
	LADSPA_Data *amplitude;
	LADSPA_Data *gain_red;
	LADSPA_Data *input;
	LADSPA_Data *output;
	float        amp;
	float *      as;
	unsigned int count;
	float        env;
	float        env_peak;
	float        env_rms;
	float        gain;
	float        gain_t;
	rms_env *    rms;
	float        sum;
	LADSPA_Data run_adding_gain;
} Sc4m;

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
		return sc4mDescriptor;
	default:
		return NULL;
	}
}

static void cleanupSc4m(LADSPA_Handle instance) {
#line 46 "sc4m_1916.xml"
	Sc4m *plugin_data = (Sc4m *)instance;
	rms_env_free(plugin_data->rms);
	free(plugin_data->as);
	free(instance);
}

static void connectPortSc4m(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Sc4m *plugin;

	plugin = (Sc4m *)instance;
	switch (port) {
	case SC4M_RMS_PEAK:
		plugin->rms_peak = data;
		break;
	case SC4M_ATTACK:
		plugin->attack = data;
		break;
	case SC4M_RELEASE:
		plugin->release = data;
		break;
	case SC4M_THRESHOLD:
		plugin->threshold = data;
		break;
	case SC4M_RATIO:
		plugin->ratio = data;
		break;
	case SC4M_KNEE:
		plugin->knee = data;
		break;
	case SC4M_MAKEUP_GAIN:
		plugin->makeup_gain = data;
		break;
	case SC4M_AMPLITUDE:
		plugin->amplitude = data;
		break;
	case SC4M_GAIN_RED:
		plugin->gain_red = data;
		break;
	case SC4M_INPUT:
		plugin->input = data;
		break;
	case SC4M_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateSc4m(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Sc4m *plugin_data = (Sc4m *)malloc(sizeof(Sc4m));
	float amp;
	float *as = NULL;
	unsigned int count;
	float env;
	float env_peak;
	float env_rms;
	float gain;
	float gain_t;
	rms_env *rms = NULL;
	float sum;

#line 23 "sc4m_1916.xml"
	unsigned int i;
	float sample_rate = (float)s_rate;

	rms = rms_env_new();
	sum = 0.0f;
	amp = 0.0f;
	gain = 0.0f;
	gain_t = 0.0f;
	env = 0.0f;
	env_rms = 0.0f;
	env_peak = 0.0f;
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
	plugin_data->env_peak = env_peak;
	plugin_data->env_rms = env_rms;
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

static void runSc4m(LADSPA_Handle instance, unsigned long sample_count) {
	Sc4m *plugin_data = (Sc4m *)instance;

	/* RMS/peak (float value) */
	const LADSPA_Data rms_peak = *(plugin_data->rms_peak);

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
	float env_peak = plugin_data->env_peak;
	float env_rms = plugin_data->env_rms;
	float gain = plugin_data->gain;
	float gain_t = plugin_data->gain_t;
	rms_env * rms = plugin_data->rms;
	float sum = plugin_data->sum;

#line 51 "sc4m_1916.xml"
	unsigned long pos;

	const float ga = attack < 2.0f ? 0.0f : as[f_round(attack * 0.001f * (float)(A_TBL-1))];
	const float gr = as[f_round(release * 0.001f * (float)(A_TBL-1))];
	const float rs = (ratio - 1.0f) / ratio;
	const float mug = db2lin(makeup_gain);
	const float knee_min = db2lin(threshold - knee);
	const float knee_max = db2lin(threshold + knee);
	const float ef_a = ga * 0.25f;
	const float ef_ai = 1.0f - ef_a;

	for (pos = 0; pos < sample_count; pos++) {
	         const float lev_in = input[pos];
	  sum += lev_in * lev_in;

	  if (amp > env_rms) {
	    env_rms = env_rms * ga + amp * (1.0f - ga);
	  } else {
	    env_rms = env_rms * gr + amp * (1.0f - gr);
	  }
	  round_to_zero(&env_rms);
	  if (lev_in > env_peak) {
	    env_peak = env_peak * ga + lev_in * (1.0f - ga);
	  } else {
	    env_peak = env_peak * gr + lev_in * (1.0f - gr);
	  }
	  round_to_zero(&env_peak);
	  if ((count++ & 3) == 3) {
	    amp = rms_env_process(rms, sum * 0.25f);
	    sum = 0.0f;

	    env = LIN_INTERP(rms_peak, env_rms, env_peak);

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
	plugin_data->env_rms = env_rms;
	plugin_data->env_peak = env_peak;
	plugin_data->count = count;

	*(plugin_data->amplitude) = lin2db(env);
	*(plugin_data->gain_red) = lin2db(gain);
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainSc4m(LADSPA_Handle instance, LADSPA_Data gain) {
	((Sc4m *)instance)->run_adding_gain = gain;
}

static void runAddingSc4m(LADSPA_Handle instance, unsigned long sample_count) {
	Sc4m *plugin_data = (Sc4m *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* RMS/peak (float value) */
	const LADSPA_Data rms_peak = *(plugin_data->rms_peak);

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
	float env_peak = plugin_data->env_peak;
	float env_rms = plugin_data->env_rms;
	float gain = plugin_data->gain;
	float gain_t = plugin_data->gain_t;
	rms_env * rms = plugin_data->rms;
	float sum = plugin_data->sum;

#line 51 "sc4m_1916.xml"
	unsigned long pos;

	const float ga = attack < 2.0f ? 0.0f : as[f_round(attack * 0.001f * (float)(A_TBL-1))];
	const float gr = as[f_round(release * 0.001f * (float)(A_TBL-1))];
	const float rs = (ratio - 1.0f) / ratio;
	const float mug = db2lin(makeup_gain);
	const float knee_min = db2lin(threshold - knee);
	const float knee_max = db2lin(threshold + knee);
	const float ef_a = ga * 0.25f;
	const float ef_ai = 1.0f - ef_a;

	for (pos = 0; pos < sample_count; pos++) {
	         const float lev_in = input[pos];
	  sum += lev_in * lev_in;

	  if (amp > env_rms) {
	    env_rms = env_rms * ga + amp * (1.0f - ga);
	  } else {
	    env_rms = env_rms * gr + amp * (1.0f - gr);
	  }
	  round_to_zero(&env_rms);
	  if (lev_in > env_peak) {
	    env_peak = env_peak * ga + lev_in * (1.0f - ga);
	  } else {
	    env_peak = env_peak * gr + lev_in * (1.0f - gr);
	  }
	  round_to_zero(&env_peak);
	  if ((count++ & 3) == 3) {
	    amp = rms_env_process(rms, sum * 0.25f);
	    sum = 0.0f;

	    env = LIN_INTERP(rms_peak, env_rms, env_peak);

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
	plugin_data->env_rms = env_rms;
	plugin_data->env_peak = env_peak;
	plugin_data->count = count;

	*(plugin_data->amplitude) = lin2db(env);
	*(plugin_data->gain_red) = lin2db(gain);
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


	sc4mDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (sc4mDescriptor) {
		sc4mDescriptor->UniqueID = 1916;
		sc4mDescriptor->Label = "sc4m";
		sc4mDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		sc4mDescriptor->Name =
		 D_("SC4 mono");
		sc4mDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		sc4mDescriptor->Copyright =
		 "GPL";
		sc4mDescriptor->PortCount = 11;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(11,
		 sizeof(LADSPA_PortDescriptor));
		sc4mDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(11,
		 sizeof(LADSPA_PortRangeHint));
		sc4mDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(11, sizeof(char*));
		sc4mDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for RMS/peak */
		port_descriptors[SC4M_RMS_PEAK] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC4M_RMS_PEAK] =
		 D_("RMS/peak");
		port_range_hints[SC4M_RMS_PEAK].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MINIMUM;
		port_range_hints[SC4M_RMS_PEAK].LowerBound = 0;
		port_range_hints[SC4M_RMS_PEAK].UpperBound = 1;

		/* Parameters for Attack time (ms) */
		port_descriptors[SC4M_ATTACK] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC4M_ATTACK] =
		 D_("Attack time (ms)");
		port_range_hints[SC4M_ATTACK].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[SC4M_ATTACK].LowerBound = 1.5;
		port_range_hints[SC4M_ATTACK].UpperBound = 400;

		/* Parameters for Release time (ms) */
		port_descriptors[SC4M_RELEASE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC4M_RELEASE] =
		 D_("Release time (ms)");
		port_range_hints[SC4M_RELEASE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[SC4M_RELEASE].LowerBound = 2;
		port_range_hints[SC4M_RELEASE].UpperBound = 800;

		/* Parameters for Threshold level (dB) */
		port_descriptors[SC4M_THRESHOLD] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC4M_THRESHOLD] =
		 D_("Threshold level (dB)");
		port_range_hints[SC4M_THRESHOLD].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MAXIMUM;
		port_range_hints[SC4M_THRESHOLD].LowerBound = -30;
		port_range_hints[SC4M_THRESHOLD].UpperBound = 0;

		/* Parameters for Ratio (1:n) */
		port_descriptors[SC4M_RATIO] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC4M_RATIO] =
		 D_("Ratio (1:n)");
		port_range_hints[SC4M_RATIO].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[SC4M_RATIO].LowerBound = 1;
		port_range_hints[SC4M_RATIO].UpperBound = 20;

		/* Parameters for Knee radius (dB) */
		port_descriptors[SC4M_KNEE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC4M_KNEE] =
		 D_("Knee radius (dB)");
		port_range_hints[SC4M_KNEE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[SC4M_KNEE].LowerBound = 1;
		port_range_hints[SC4M_KNEE].UpperBound = 10;

		/* Parameters for Makeup gain (dB) */
		port_descriptors[SC4M_MAKEUP_GAIN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SC4M_MAKEUP_GAIN] =
		 D_("Makeup gain (dB)");
		port_range_hints[SC4M_MAKEUP_GAIN].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[SC4M_MAKEUP_GAIN].LowerBound = 0;
		port_range_hints[SC4M_MAKEUP_GAIN].UpperBound = +24;

		/* Parameters for Amplitude (dB) */
		port_descriptors[SC4M_AMPLITUDE] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
		port_names[SC4M_AMPLITUDE] =
		 D_("Amplitude (dB)");
		port_range_hints[SC4M_AMPLITUDE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[SC4M_AMPLITUDE].LowerBound = -40;
		port_range_hints[SC4M_AMPLITUDE].UpperBound = +12;

		/* Parameters for Gain reduction (dB) */
		port_descriptors[SC4M_GAIN_RED] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
		port_names[SC4M_GAIN_RED] =
		 D_("Gain reduction (dB)");
		port_range_hints[SC4M_GAIN_RED].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[SC4M_GAIN_RED].LowerBound = -24;
		port_range_hints[SC4M_GAIN_RED].UpperBound = 0;

		/* Parameters for Input */
		port_descriptors[SC4M_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[SC4M_INPUT] =
		 D_("Input");
		port_range_hints[SC4M_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[SC4M_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[SC4M_OUTPUT] =
		 D_("Output");
		port_range_hints[SC4M_OUTPUT].HintDescriptor = 0;

		sc4mDescriptor->activate = NULL;
		sc4mDescriptor->cleanup = cleanupSc4m;
		sc4mDescriptor->connect_port = connectPortSc4m;
		sc4mDescriptor->deactivate = NULL;
		sc4mDescriptor->instantiate = instantiateSc4m;
		sc4mDescriptor->run = runSc4m;
		sc4mDescriptor->run_adding = runAddingSc4m;
		sc4mDescriptor->set_run_adding_gain = setRunAddingGainSc4m;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (sc4mDescriptor) {
		free((LADSPA_PortDescriptor *)sc4mDescriptor->PortDescriptors);
		free((char **)sc4mDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)sc4mDescriptor->PortRangeHints);
		free(sc4mDescriptor);
	}

}
