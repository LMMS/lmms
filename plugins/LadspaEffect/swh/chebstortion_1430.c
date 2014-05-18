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

#line 10 "chebstortion_1430.xml"

#include <ladspa-util.h>

#define HARMONICS 11
#define STAGES 2

static float cd_lut[STAGES][HARMONICS];

/* Calculate Chebychev coefficents from partial magnitudes, adapted from
 * example in Num. Rec. */
void chebpc(float c[], float d[])
{
    int k, j;
    float sv, dd[HARMONICS];

    for (j = 0; j < HARMONICS; j++) {
        d[j] = dd[j] = 0.0;
    }

    d[0] = c[HARMONICS - 1];

    for (j = HARMONICS - 2; j >= 1; j--) {
        for (k = HARMONICS - j; k >= 1; k--) {
            sv = d[k];
            d[k] = 2.0 * d[k - 1] - dd[k];
            dd[k] = sv;
        }
        sv = d[0];
        d[0] = -dd[0] + c[j];
        dd[0] = sv;
    }

    for (j = HARMONICS - 1; j >= 1; j--) {
        d[j] = d[j - 1] - dd[j];
    }
    d[0] = -dd[0] + 0.5 * c[0];
}

#define CHEBSTORTION_DIST              0
#define CHEBSTORTION_INPUT             1
#define CHEBSTORTION_OUTPUT            2

static LADSPA_Descriptor *chebstortionDescriptor = NULL;

typedef struct {
	LADSPA_Data *dist;
	LADSPA_Data *input;
	LADSPA_Data *output;
	unsigned int count;
	float        env;
	float        itm1;
	float        otm1;
	LADSPA_Data run_adding_gain;
} Chebstortion;

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
		return chebstortionDescriptor;
	default:
		return NULL;
	}
}

static void activateChebstortion(LADSPA_Handle instance) {
	Chebstortion *plugin_data = (Chebstortion *)instance;
	unsigned int count = plugin_data->count;
	float env = plugin_data->env;
	float itm1 = plugin_data->itm1;
	float otm1 = plugin_data->otm1;
#line 82 "chebstortion_1430.xml"
	itm1 = 0.0f;
	otm1 = 0.0f;
	env = 0.0f;
	count = 0;
	plugin_data->count = count;
	plugin_data->env = env;
	plugin_data->itm1 = itm1;
	plugin_data->otm1 = otm1;

}

static void cleanupChebstortion(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortChebstortion(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Chebstortion *plugin;

	plugin = (Chebstortion *)instance;
	switch (port) {
	case CHEBSTORTION_DIST:
		plugin->dist = data;
		break;
	case CHEBSTORTION_INPUT:
		plugin->input = data;
		break;
	case CHEBSTORTION_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateChebstortion(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Chebstortion *plugin_data = (Chebstortion *)malloc(sizeof(Chebstortion));
	unsigned int count;
	float env;
	float itm1;
	float otm1;

#line 62 "chebstortion_1430.xml"
	unsigned int i;

	cd_lut[0][0] = 0.0f;
	cd_lut[0][1] = 1.0f;
	for (i=2; i<HARMONICS; i++) {
	  cd_lut[0][i] = 0.0f;
	}
	cd_lut[1][0] = 0.0f;
	cd_lut[1][1] = 1.0f;
	for (i=2; i<HARMONICS; i++) {
	  cd_lut[1][i] = 1.0f/(float)i;
	}

	itm1 = 0.0f;
	otm1 = 0.0f;
	env = 0.0f;
	count = 0;

	plugin_data->count = count;
	plugin_data->env = env;
	plugin_data->itm1 = itm1;
	plugin_data->otm1 = otm1;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runChebstortion(LADSPA_Handle instance, unsigned long sample_count) {
	Chebstortion *plugin_data = (Chebstortion *)instance;

	/* Distortion (float value) */
	const LADSPA_Data dist = *(plugin_data->dist);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	unsigned int count = plugin_data->count;
	float env = plugin_data->env;
	float itm1 = plugin_data->itm1;
	float otm1 = plugin_data->otm1;

#line 89 "chebstortion_1430.xml"
	unsigned long pos, i;
	float p[HARMONICS], interp[HARMONICS];

	for (pos = 0; pos < sample_count; pos++) {
	  const float x = input[pos];
	  const float a = fabs(input[pos]);
	  float y;

	  if (a > env) {
	          env = env * 0.9f + a * 0.1f;
	  } else {
	          env = env * 0.97f + a * 0.03f;
	  }

	  if (count-- == 0) {
	    for (i=0; i<HARMONICS; i++) {
	      interp[i] = cd_lut[0][i] * (1.0f - env * dist) +
	                  cd_lut[1][i] * env * dist;
	    }
	    chebpc(interp, p);

	    count = 4;
	  }

	  // Evaluate the polynomial using Horner's Rule
	  y = p[0] + (p[1] + (p[2] + (p[3] + (p[4] + (p[5] + (p[6] + (p[7] +
	      (p[8] + (p[9] + p[10] * x) * x) * x) * x) * x) * x) * x) * x) *
	      x) * x;

	  // DC offset remove (odd harmonics cause DC offset)
	  otm1 = 0.999f * otm1 + y - itm1;
	  itm1 = y;

	  buffer_write(output[pos], otm1);
	}

	plugin_data->itm1 = itm1;
	plugin_data->otm1 = otm1;
	plugin_data->env = env;
	plugin_data->count = count;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainChebstortion(LADSPA_Handle instance, LADSPA_Data gain) {
	((Chebstortion *)instance)->run_adding_gain = gain;
}

static void runAddingChebstortion(LADSPA_Handle instance, unsigned long sample_count) {
	Chebstortion *plugin_data = (Chebstortion *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Distortion (float value) */
	const LADSPA_Data dist = *(plugin_data->dist);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	unsigned int count = plugin_data->count;
	float env = plugin_data->env;
	float itm1 = plugin_data->itm1;
	float otm1 = plugin_data->otm1;

#line 89 "chebstortion_1430.xml"
	unsigned long pos, i;
	float p[HARMONICS], interp[HARMONICS];

	for (pos = 0; pos < sample_count; pos++) {
	  const float x = input[pos];
	  const float a = fabs(input[pos]);
	  float y;

	  if (a > env) {
	          env = env * 0.9f + a * 0.1f;
	  } else {
	          env = env * 0.97f + a * 0.03f;
	  }

	  if (count-- == 0) {
	    for (i=0; i<HARMONICS; i++) {
	      interp[i] = cd_lut[0][i] * (1.0f - env * dist) +
	                  cd_lut[1][i] * env * dist;
	    }
	    chebpc(interp, p);

	    count = 4;
	  }

	  // Evaluate the polynomial using Horner's Rule
	  y = p[0] + (p[1] + (p[2] + (p[3] + (p[4] + (p[5] + (p[6] + (p[7] +
	      (p[8] + (p[9] + p[10] * x) * x) * x) * x) * x) * x) * x) * x) *
	      x) * x;

	  // DC offset remove (odd harmonics cause DC offset)
	  otm1 = 0.999f * otm1 + y - itm1;
	  itm1 = y;

	  buffer_write(output[pos], otm1);
	}

	plugin_data->itm1 = itm1;
	plugin_data->otm1 = otm1;
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


	chebstortionDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (chebstortionDescriptor) {
		chebstortionDescriptor->UniqueID = 1430;
		chebstortionDescriptor->Label = "chebstortion";
		chebstortionDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		chebstortionDescriptor->Name =
		 D_("Chebyshev distortion");
		chebstortionDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		chebstortionDescriptor->Copyright =
		 "GPL";
		chebstortionDescriptor->PortCount = 3;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(3,
		 sizeof(LADSPA_PortDescriptor));
		chebstortionDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(3,
		 sizeof(LADSPA_PortRangeHint));
		chebstortionDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(3, sizeof(char*));
		chebstortionDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Distortion */
		port_descriptors[CHEBSTORTION_DIST] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[CHEBSTORTION_DIST] =
		 D_("Distortion");
		port_range_hints[CHEBSTORTION_DIST].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[CHEBSTORTION_DIST].LowerBound = 0;
		port_range_hints[CHEBSTORTION_DIST].UpperBound = 3;

		/* Parameters for Input */
		port_descriptors[CHEBSTORTION_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[CHEBSTORTION_INPUT] =
		 D_("Input");
		port_range_hints[CHEBSTORTION_INPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[CHEBSTORTION_INPUT].LowerBound = -1;
		port_range_hints[CHEBSTORTION_INPUT].UpperBound = +1;

		/* Parameters for Output */
		port_descriptors[CHEBSTORTION_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[CHEBSTORTION_OUTPUT] =
		 D_("Output");
		port_range_hints[CHEBSTORTION_OUTPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[CHEBSTORTION_OUTPUT].LowerBound = -1;
		port_range_hints[CHEBSTORTION_OUTPUT].UpperBound = +1;

		chebstortionDescriptor->activate = activateChebstortion;
		chebstortionDescriptor->cleanup = cleanupChebstortion;
		chebstortionDescriptor->connect_port = connectPortChebstortion;
		chebstortionDescriptor->deactivate = NULL;
		chebstortionDescriptor->instantiate = instantiateChebstortion;
		chebstortionDescriptor->run = runChebstortion;
		chebstortionDescriptor->run_adding = runAddingChebstortion;
		chebstortionDescriptor->set_run_adding_gain = setRunAddingGainChebstortion;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (chebstortionDescriptor) {
		free((LADSPA_PortDescriptor *)chebstortionDescriptor->PortDescriptors);
		free((char **)chebstortionDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)chebstortionDescriptor->PortRangeHints);
		free(chebstortionDescriptor);
	}

}
