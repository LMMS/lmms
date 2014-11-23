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

#line 10 "se4_1883.xml"

#include "util/db.h"
#include "util/rms.h"

#define A_TBL 256

#define SE4_RMS_PEAK                   0
#define SE4_ATTACK                     1
#define SE4_RELEASE                    2
#define SE4_THRESHOLD                  3
#define SE4_RATIO                      4
#define SE4_KNEE                       5
#define SE4_ATTENUATION                6
#define SE4_AMPLITUDE                  7
#define SE4_GAIN_EXP                   8
#define SE4_LEFT_IN                    9
#define SE4_RIGHT_IN                   10
#define SE4_LEFT_OUT                   11
#define SE4_RIGHT_OUT                  12

static LADSPA_Descriptor *se4Descriptor = NULL;

typedef struct {
	LADSPA_Data *rms_peak;
	LADSPA_Data *attack;
	LADSPA_Data *release;
	LADSPA_Data *threshold;
	LADSPA_Data *ratio;
	LADSPA_Data *knee;
	LADSPA_Data *attenuation;
	LADSPA_Data *amplitude;
	LADSPA_Data *gain_exp;
	LADSPA_Data *left_in;
	LADSPA_Data *right_in;
	LADSPA_Data *left_out;
	LADSPA_Data *right_out;
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
} Se4;

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
		return se4Descriptor;
	default:
		return NULL;
	}
}

static void cleanupSe4(LADSPA_Handle instance) {
#line 46 "se4_1883.xml"
	Se4 *plugin_data = (Se4 *)instance;
	rms_env_free(plugin_data->rms);
	free(plugin_data->as);
	free(instance);
}

static void connectPortSe4(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Se4 *plugin;

	plugin = (Se4 *)instance;
	switch (port) {
	case SE4_RMS_PEAK:
		plugin->rms_peak = data;
		break;
	case SE4_ATTACK:
		plugin->attack = data;
		break;
	case SE4_RELEASE:
		plugin->release = data;
		break;
	case SE4_THRESHOLD:
		plugin->threshold = data;
		break;
	case SE4_RATIO:
		plugin->ratio = data;
		break;
	case SE4_KNEE:
		plugin->knee = data;
		break;
	case SE4_ATTENUATION:
		plugin->attenuation = data;
		break;
	case SE4_AMPLITUDE:
		plugin->amplitude = data;
		break;
	case SE4_GAIN_EXP:
		plugin->gain_exp = data;
		break;
	case SE4_LEFT_IN:
		plugin->left_in = data;
		break;
	case SE4_RIGHT_IN:
		plugin->right_in = data;
		break;
	case SE4_LEFT_OUT:
		plugin->left_out = data;
		break;
	case SE4_RIGHT_OUT:
		plugin->right_out = data;
		break;
	}
}

static LADSPA_Handle instantiateSe4(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Se4 *plugin_data = (Se4 *)malloc(sizeof(Se4));
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

#line 23 "se4_1883.xml"
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

static void runSe4(LADSPA_Handle instance, unsigned long sample_count) {
	Se4 *plugin_data = (Se4 *)instance;

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

	/* Attenuation (dB) (float value) */
	const LADSPA_Data attenuation = *(plugin_data->attenuation);

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
	float env_peak = plugin_data->env_peak;
	float env_rms = plugin_data->env_rms;
	float gain = plugin_data->gain;
	float gain_t = plugin_data->gain_t;
	rms_env * rms = plugin_data->rms;
	float sum = plugin_data->sum;

#line 51 "se4_1883.xml"
	unsigned long pos;

	const float ga = attack < 2.0f ? 0.0f : as[f_round(attack * 0.001f * (float)(A_TBL-1))];
	const float gr = as[f_round(release * 0.001f * (float)(A_TBL-1))];
	const float rs = ratio / (ratio - 1.0f);
	const float mug = db2lin(attenuation);
	const float knee_min = db2lin(threshold - knee);
	const float knee_max = db2lin(threshold + knee);
	const float ef_a = ga * 0.25f;
	const float ef_ai = 1.0f - ef_a;

	for (pos = 0; pos < sample_count; pos++) {
	  const float la = fabs(left_in[pos]);
	  const float ra = fabs(right_in[pos]);
	  const float lev_in = f_max(la, ra);
	  sum += lev_in * lev_in;

	  if (amp > env_rms) {
	    env_rms = env_rms * ga + amp * (1.0f - ga);
	  } else {
	    env_rms = env_rms * gr + amp * (1.0f - gr);
	  }
	  if (lev_in > env_peak) {
	    env_peak = env_peak * ga + lev_in * (1.0f - ga);
	  } else {
	    env_peak = env_peak * gr + lev_in * (1.0f - gr);
	  }
	  if ((count++ & 3) == 3) {
	    amp = rms_env_process(rms, sum * 0.25f);
	    sum = 0.0f;
	    if (isnan(env_rms)) {
	      // This can happen sometimes, but I don't know why
	      env_rms = 0.0f;
	    }

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
	  buffer_write(left_out[pos], left_in[pos] * gain * mug);
	  buffer_write(right_out[pos], right_in[pos] * gain * mug);
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
	*(plugin_data->gain_exp) = lin2db(gain);
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainSe4(LADSPA_Handle instance, LADSPA_Data gain) {
	((Se4 *)instance)->run_adding_gain = gain;
}

static void runAddingSe4(LADSPA_Handle instance, unsigned long sample_count) {
	Se4 *plugin_data = (Se4 *)instance;
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

	/* Attenuation (dB) (float value) */
	const LADSPA_Data attenuation = *(plugin_data->attenuation);

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
	float env_peak = plugin_data->env_peak;
	float env_rms = plugin_data->env_rms;
	float gain = plugin_data->gain;
	float gain_t = plugin_data->gain_t;
	rms_env * rms = plugin_data->rms;
	float sum = plugin_data->sum;

#line 51 "se4_1883.xml"
	unsigned long pos;

	const float ga = attack < 2.0f ? 0.0f : as[f_round(attack * 0.001f * (float)(A_TBL-1))];
	const float gr = as[f_round(release * 0.001f * (float)(A_TBL-1))];
	const float rs = ratio / (ratio - 1.0f);
	const float mug = db2lin(attenuation);
	const float knee_min = db2lin(threshold - knee);
	const float knee_max = db2lin(threshold + knee);
	const float ef_a = ga * 0.25f;
	const float ef_ai = 1.0f - ef_a;

	for (pos = 0; pos < sample_count; pos++) {
	  const float la = fabs(left_in[pos]);
	  const float ra = fabs(right_in[pos]);
	  const float lev_in = f_max(la, ra);
	  sum += lev_in * lev_in;

	  if (amp > env_rms) {
	    env_rms = env_rms * ga + amp * (1.0f - ga);
	  } else {
	    env_rms = env_rms * gr + amp * (1.0f - gr);
	  }
	  if (lev_in > env_peak) {
	    env_peak = env_peak * ga + lev_in * (1.0f - ga);
	  } else {
	    env_peak = env_peak * gr + lev_in * (1.0f - gr);
	  }
	  if ((count++ & 3) == 3) {
	    amp = rms_env_process(rms, sum * 0.25f);
	    sum = 0.0f;
	    if (isnan(env_rms)) {
	      // This can happen sometimes, but I don't know why
	      env_rms = 0.0f;
	    }

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
	  buffer_write(left_out[pos], left_in[pos] * gain * mug);
	  buffer_write(right_out[pos], right_in[pos] * gain * mug);
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
	*(plugin_data->gain_exp) = lin2db(gain);
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


	se4Descriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (se4Descriptor) {
		se4Descriptor->UniqueID = 1883;
		se4Descriptor->Label = "se4";
		se4Descriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		se4Descriptor->Name =
		 D_("SE4");
		se4Descriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		se4Descriptor->Copyright =
		 "GPL";
		se4Descriptor->PortCount = 13;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(13,
		 sizeof(LADSPA_PortDescriptor));
		se4Descriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(13,
		 sizeof(LADSPA_PortRangeHint));
		se4Descriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(13, sizeof(char*));
		se4Descriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for RMS/peak */
		port_descriptors[SE4_RMS_PEAK] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SE4_RMS_PEAK] =
		 D_("RMS/peak");
		port_range_hints[SE4_RMS_PEAK].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MINIMUM;
		port_range_hints[SE4_RMS_PEAK].LowerBound = 0;
		port_range_hints[SE4_RMS_PEAK].UpperBound = 1;

		/* Parameters for Attack time (ms) */
		port_descriptors[SE4_ATTACK] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SE4_ATTACK] =
		 D_("Attack time (ms)");
		port_range_hints[SE4_ATTACK].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[SE4_ATTACK].LowerBound = 1.5;
		port_range_hints[SE4_ATTACK].UpperBound = 400;

		/* Parameters for Release time (ms) */
		port_descriptors[SE4_RELEASE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SE4_RELEASE] =
		 D_("Release time (ms)");
		port_range_hints[SE4_RELEASE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[SE4_RELEASE].LowerBound = 2;
		port_range_hints[SE4_RELEASE].UpperBound = 800;

		/* Parameters for Threshold level (dB) */
		port_descriptors[SE4_THRESHOLD] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SE4_THRESHOLD] =
		 D_("Threshold level (dB)");
		port_range_hints[SE4_THRESHOLD].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MAXIMUM;
		port_range_hints[SE4_THRESHOLD].LowerBound = -30;
		port_range_hints[SE4_THRESHOLD].UpperBound = 0;

		/* Parameters for Ratio (1:n) */
		port_descriptors[SE4_RATIO] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SE4_RATIO] =
		 D_("Ratio (1:n)");
		port_range_hints[SE4_RATIO].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[SE4_RATIO].LowerBound = 1;
		port_range_hints[SE4_RATIO].UpperBound = 20;

		/* Parameters for Knee radius (dB) */
		port_descriptors[SE4_KNEE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SE4_KNEE] =
		 D_("Knee radius (dB)");
		port_range_hints[SE4_KNEE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[SE4_KNEE].LowerBound = 1;
		port_range_hints[SE4_KNEE].UpperBound = 10;

		/* Parameters for Attenuation (dB) */
		port_descriptors[SE4_ATTENUATION] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SE4_ATTENUATION] =
		 D_("Attenuation (dB)");
		port_range_hints[SE4_ATTENUATION].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[SE4_ATTENUATION].LowerBound = -24;
		port_range_hints[SE4_ATTENUATION].UpperBound = 0;

		/* Parameters for Amplitude (dB) */
		port_descriptors[SE4_AMPLITUDE] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
		port_names[SE4_AMPLITUDE] =
		 D_("Amplitude (dB)");
		port_range_hints[SE4_AMPLITUDE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[SE4_AMPLITUDE].LowerBound = -40;
		port_range_hints[SE4_AMPLITUDE].UpperBound = +12;

		/* Parameters for Gain expansion (dB) */
		port_descriptors[SE4_GAIN_EXP] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
		port_names[SE4_GAIN_EXP] =
		 D_("Gain expansion (dB)");
		port_range_hints[SE4_GAIN_EXP].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[SE4_GAIN_EXP].LowerBound = 0;
		port_range_hints[SE4_GAIN_EXP].UpperBound = +24;

		/* Parameters for Left input */
		port_descriptors[SE4_LEFT_IN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[SE4_LEFT_IN] =
		 D_("Left input");
		port_range_hints[SE4_LEFT_IN].HintDescriptor = 0;

		/* Parameters for Right input */
		port_descriptors[SE4_RIGHT_IN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[SE4_RIGHT_IN] =
		 D_("Right input");
		port_range_hints[SE4_RIGHT_IN].HintDescriptor = 0;

		/* Parameters for Left output */
		port_descriptors[SE4_LEFT_OUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[SE4_LEFT_OUT] =
		 D_("Left output");
		port_range_hints[SE4_LEFT_OUT].HintDescriptor = 0;

		/* Parameters for Right output */
		port_descriptors[SE4_RIGHT_OUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[SE4_RIGHT_OUT] =
		 D_("Right output");
		port_range_hints[SE4_RIGHT_OUT].HintDescriptor = 0;

		se4Descriptor->activate = NULL;
		se4Descriptor->cleanup = cleanupSe4;
		se4Descriptor->connect_port = connectPortSe4;
		se4Descriptor->deactivate = NULL;
		se4Descriptor->instantiate = instantiateSe4;
		se4Descriptor->run = runSe4;
		se4Descriptor->run_adding = runAddingSe4;
		se4Descriptor->set_run_adding_gain = setRunAddingGainSe4;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (se4Descriptor) {
		free((LADSPA_PortDescriptor *)se4Descriptor->PortDescriptors);
		free((char **)se4Descriptor->PortNames);
		free((LADSPA_PortRangeHint *)se4Descriptor->PortRangeHints);
		free(se4Descriptor);
	}

}
