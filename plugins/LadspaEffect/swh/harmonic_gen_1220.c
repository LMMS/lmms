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

#line 10 "harmonic_gen_1220.xml"

#define HARMONICS 11

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

#define HARMONICGEN_MAG_1              0
#define HARMONICGEN_MAG_2              1
#define HARMONICGEN_MAG_3              2
#define HARMONICGEN_MAG_4              3
#define HARMONICGEN_MAG_5              4
#define HARMONICGEN_MAG_6              5
#define HARMONICGEN_MAG_7              6
#define HARMONICGEN_MAG_8              7
#define HARMONICGEN_MAG_9              8
#define HARMONICGEN_MAG_10             9
#define HARMONICGEN_INPUT              10
#define HARMONICGEN_OUTPUT             11

static LADSPA_Descriptor *harmonicGenDescriptor = NULL;

typedef struct {
	LADSPA_Data *mag_1;
	LADSPA_Data *mag_2;
	LADSPA_Data *mag_3;
	LADSPA_Data *mag_4;
	LADSPA_Data *mag_5;
	LADSPA_Data *mag_6;
	LADSPA_Data *mag_7;
	LADSPA_Data *mag_8;
	LADSPA_Data *mag_9;
	LADSPA_Data *mag_10;
	LADSPA_Data *input;
	LADSPA_Data *output;
	float        itm1;
	float        otm1;
	LADSPA_Data run_adding_gain;
} HarmonicGen;

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
		return harmonicGenDescriptor;
	default:
		return NULL;
	}
}

static void activateHarmonicGen(LADSPA_Handle instance) {
	HarmonicGen *plugin_data = (HarmonicGen *)instance;
	float itm1 = plugin_data->itm1;
	float otm1 = plugin_data->otm1;
#line 56 "harmonic_gen_1220.xml"
	itm1 = 0.0f;
	otm1 = 0.0f;
	plugin_data->itm1 = itm1;
	plugin_data->otm1 = otm1;

}

static void cleanupHarmonicGen(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortHarmonicGen(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	HarmonicGen *plugin;

	plugin = (HarmonicGen *)instance;
	switch (port) {
	case HARMONICGEN_MAG_1:
		plugin->mag_1 = data;
		break;
	case HARMONICGEN_MAG_2:
		plugin->mag_2 = data;
		break;
	case HARMONICGEN_MAG_3:
		plugin->mag_3 = data;
		break;
	case HARMONICGEN_MAG_4:
		plugin->mag_4 = data;
		break;
	case HARMONICGEN_MAG_5:
		plugin->mag_5 = data;
		break;
	case HARMONICGEN_MAG_6:
		plugin->mag_6 = data;
		break;
	case HARMONICGEN_MAG_7:
		plugin->mag_7 = data;
		break;
	case HARMONICGEN_MAG_8:
		plugin->mag_8 = data;
		break;
	case HARMONICGEN_MAG_9:
		plugin->mag_9 = data;
		break;
	case HARMONICGEN_MAG_10:
		plugin->mag_10 = data;
		break;
	case HARMONICGEN_INPUT:
		plugin->input = data;
		break;
	case HARMONICGEN_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateHarmonicGen(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	HarmonicGen *plugin_data = (HarmonicGen *)malloc(sizeof(HarmonicGen));
	plugin_data->run_adding_gain = 1.0f;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runHarmonicGen(LADSPA_Handle instance, unsigned long sample_count) {
	HarmonicGen *plugin_data = (HarmonicGen *)instance;

	/* Fundamental magnitude (float value) */
	const LADSPA_Data mag_1 = *(plugin_data->mag_1);

	/* 2nd harmonic magnitude (float value) */
	const LADSPA_Data mag_2 = *(plugin_data->mag_2);

	/* 3rd harmonic magnitude (float value) */
	const LADSPA_Data mag_3 = *(plugin_data->mag_3);

	/* 4th harmonic magnitude (float value) */
	const LADSPA_Data mag_4 = *(plugin_data->mag_4);

	/* 5th harmonic magnitude (float value) */
	const LADSPA_Data mag_5 = *(plugin_data->mag_5);

	/* 6th harmonic magnitude (float value) */
	const LADSPA_Data mag_6 = *(plugin_data->mag_6);

	/* 7th harmonic magnitude (float value) */
	const LADSPA_Data mag_7 = *(plugin_data->mag_7);

	/* 8th harmonic magnitude (float value) */
	const LADSPA_Data mag_8 = *(plugin_data->mag_8);

	/* 9th harmonic magnitude (float value) */
	const LADSPA_Data mag_9 = *(plugin_data->mag_9);

	/* 10th harmonic magnitude (float value) */
	const LADSPA_Data mag_10 = *(plugin_data->mag_10);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	float itm1 = plugin_data->itm1;
	float otm1 = plugin_data->otm1;

#line 61 "harmonic_gen_1220.xml"
	unsigned long pos, i;
	float mag_fix;
	float mag[HARMONICS] = {0.0f, mag_1, mag_2, mag_3, mag_4, mag_5, mag_6,
	                        mag_7, mag_8, mag_9, mag_10};
	float p[HARMONICS];

	// Normalise magnitudes
	mag_fix = (fabs(mag_1) + fabs(mag_2) + fabs(mag_3) + fabs(mag_4) +
	           fabs(mag_5) + fabs(mag_6) + fabs(mag_7) + fabs(mag_8) +
	           fabs(mag_9) + fabs(mag_10));
	if (mag_fix < 1.0f) {
	  mag_fix = 1.0f;
	} else {
	  mag_fix = 1.0f / mag_fix;
	}
	for (i=0; i<HARMONICS; i++) {
	  mag[i] *= mag_fix;
	}

	// Calculate polynomial coefficients, using Chebychev aproximation
	chebpc(mag, p);

	for (pos = 0; pos < sample_count; pos++) {
	  float x = input[pos], y;

	  // Calculate the polynomial using Horner's Rule
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
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainHarmonicGen(LADSPA_Handle instance, LADSPA_Data gain) {
	((HarmonicGen *)instance)->run_adding_gain = gain;
}

static void runAddingHarmonicGen(LADSPA_Handle instance, unsigned long sample_count) {
	HarmonicGen *plugin_data = (HarmonicGen *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Fundamental magnitude (float value) */
	const LADSPA_Data mag_1 = *(plugin_data->mag_1);

	/* 2nd harmonic magnitude (float value) */
	const LADSPA_Data mag_2 = *(plugin_data->mag_2);

	/* 3rd harmonic magnitude (float value) */
	const LADSPA_Data mag_3 = *(plugin_data->mag_3);

	/* 4th harmonic magnitude (float value) */
	const LADSPA_Data mag_4 = *(plugin_data->mag_4);

	/* 5th harmonic magnitude (float value) */
	const LADSPA_Data mag_5 = *(plugin_data->mag_5);

	/* 6th harmonic magnitude (float value) */
	const LADSPA_Data mag_6 = *(plugin_data->mag_6);

	/* 7th harmonic magnitude (float value) */
	const LADSPA_Data mag_7 = *(plugin_data->mag_7);

	/* 8th harmonic magnitude (float value) */
	const LADSPA_Data mag_8 = *(plugin_data->mag_8);

	/* 9th harmonic magnitude (float value) */
	const LADSPA_Data mag_9 = *(plugin_data->mag_9);

	/* 10th harmonic magnitude (float value) */
	const LADSPA_Data mag_10 = *(plugin_data->mag_10);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	float itm1 = plugin_data->itm1;
	float otm1 = plugin_data->otm1;

#line 61 "harmonic_gen_1220.xml"
	unsigned long pos, i;
	float mag_fix;
	float mag[HARMONICS] = {0.0f, mag_1, mag_2, mag_3, mag_4, mag_5, mag_6,
	                        mag_7, mag_8, mag_9, mag_10};
	float p[HARMONICS];

	// Normalise magnitudes
	mag_fix = (fabs(mag_1) + fabs(mag_2) + fabs(mag_3) + fabs(mag_4) +
	           fabs(mag_5) + fabs(mag_6) + fabs(mag_7) + fabs(mag_8) +
	           fabs(mag_9) + fabs(mag_10));
	if (mag_fix < 1.0f) {
	  mag_fix = 1.0f;
	} else {
	  mag_fix = 1.0f / mag_fix;
	}
	for (i=0; i<HARMONICS; i++) {
	  mag[i] *= mag_fix;
	}

	// Calculate polynomial coefficients, using Chebychev aproximation
	chebpc(mag, p);

	for (pos = 0; pos < sample_count; pos++) {
	  float x = input[pos], y;

	  // Calculate the polynomial using Horner's Rule
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


	harmonicGenDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (harmonicGenDescriptor) {
		harmonicGenDescriptor->UniqueID = 1220;
		harmonicGenDescriptor->Label = "harmonicGen";
		harmonicGenDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		harmonicGenDescriptor->Name =
		 D_("Harmonic generator");
		harmonicGenDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		harmonicGenDescriptor->Copyright =
		 "GPL";
		harmonicGenDescriptor->PortCount = 12;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(12,
		 sizeof(LADSPA_PortDescriptor));
		harmonicGenDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(12,
		 sizeof(LADSPA_PortRangeHint));
		harmonicGenDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(12, sizeof(char*));
		harmonicGenDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Fundamental magnitude */
		port_descriptors[HARMONICGEN_MAG_1] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HARMONICGEN_MAG_1] =
		 D_("Fundamental magnitude");
		port_range_hints[HARMONICGEN_MAG_1].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[HARMONICGEN_MAG_1].LowerBound = -1;
		port_range_hints[HARMONICGEN_MAG_1].UpperBound = +1;

		/* Parameters for 2nd harmonic magnitude */
		port_descriptors[HARMONICGEN_MAG_2] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HARMONICGEN_MAG_2] =
		 D_("2nd harmonic magnitude");
		port_range_hints[HARMONICGEN_MAG_2].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HARMONICGEN_MAG_2].LowerBound = -1;
		port_range_hints[HARMONICGEN_MAG_2].UpperBound = +1;

		/* Parameters for 3rd harmonic magnitude */
		port_descriptors[HARMONICGEN_MAG_3] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HARMONICGEN_MAG_3] =
		 D_("3rd harmonic magnitude");
		port_range_hints[HARMONICGEN_MAG_3].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HARMONICGEN_MAG_3].LowerBound = -1;
		port_range_hints[HARMONICGEN_MAG_3].UpperBound = +1;

		/* Parameters for 4th harmonic magnitude */
		port_descriptors[HARMONICGEN_MAG_4] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HARMONICGEN_MAG_4] =
		 D_("4th harmonic magnitude");
		port_range_hints[HARMONICGEN_MAG_4].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HARMONICGEN_MAG_4].LowerBound = -1;
		port_range_hints[HARMONICGEN_MAG_4].UpperBound = +1;

		/* Parameters for 5th harmonic magnitude */
		port_descriptors[HARMONICGEN_MAG_5] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HARMONICGEN_MAG_5] =
		 D_("5th harmonic magnitude");
		port_range_hints[HARMONICGEN_MAG_5].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HARMONICGEN_MAG_5].LowerBound = -1;
		port_range_hints[HARMONICGEN_MAG_5].UpperBound = +1;

		/* Parameters for 6th harmonic magnitude */
		port_descriptors[HARMONICGEN_MAG_6] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HARMONICGEN_MAG_6] =
		 D_("6th harmonic magnitude");
		port_range_hints[HARMONICGEN_MAG_6].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HARMONICGEN_MAG_6].LowerBound = -1;
		port_range_hints[HARMONICGEN_MAG_6].UpperBound = +1;

		/* Parameters for 7th harmonic magnitude */
		port_descriptors[HARMONICGEN_MAG_7] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HARMONICGEN_MAG_7] =
		 D_("7th harmonic magnitude");
		port_range_hints[HARMONICGEN_MAG_7].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HARMONICGEN_MAG_7].LowerBound = -1;
		port_range_hints[HARMONICGEN_MAG_7].UpperBound = +1;

		/* Parameters for 8th harmonic magnitude */
		port_descriptors[HARMONICGEN_MAG_8] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HARMONICGEN_MAG_8] =
		 D_("8th harmonic magnitude");
		port_range_hints[HARMONICGEN_MAG_8].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HARMONICGEN_MAG_8].LowerBound = -1;
		port_range_hints[HARMONICGEN_MAG_8].UpperBound = +1;

		/* Parameters for 9th harmonic magnitude */
		port_descriptors[HARMONICGEN_MAG_9] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HARMONICGEN_MAG_9] =
		 D_("9th harmonic magnitude");
		port_range_hints[HARMONICGEN_MAG_9].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HARMONICGEN_MAG_9].LowerBound = -1;
		port_range_hints[HARMONICGEN_MAG_9].UpperBound = +1;

		/* Parameters for 10th harmonic magnitude */
		port_descriptors[HARMONICGEN_MAG_10] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HARMONICGEN_MAG_10] =
		 D_("10th harmonic magnitude");
		port_range_hints[HARMONICGEN_MAG_10].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HARMONICGEN_MAG_10].LowerBound = -1;
		port_range_hints[HARMONICGEN_MAG_10].UpperBound = +1;

		/* Parameters for Input */
		port_descriptors[HARMONICGEN_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[HARMONICGEN_INPUT] =
		 D_("Input");
		port_range_hints[HARMONICGEN_INPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[HARMONICGEN_INPUT].LowerBound = -1;
		port_range_hints[HARMONICGEN_INPUT].UpperBound = +1;

		/* Parameters for Output */
		port_descriptors[HARMONICGEN_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[HARMONICGEN_OUTPUT] =
		 D_("Output");
		port_range_hints[HARMONICGEN_OUTPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[HARMONICGEN_OUTPUT].LowerBound = -1;
		port_range_hints[HARMONICGEN_OUTPUT].UpperBound = +1;

		harmonicGenDescriptor->activate = activateHarmonicGen;
		harmonicGenDescriptor->cleanup = cleanupHarmonicGen;
		harmonicGenDescriptor->connect_port = connectPortHarmonicGen;
		harmonicGenDescriptor->deactivate = NULL;
		harmonicGenDescriptor->instantiate = instantiateHarmonicGen;
		harmonicGenDescriptor->run = runHarmonicGen;
		harmonicGenDescriptor->run_adding = runAddingHarmonicGen;
		harmonicGenDescriptor->set_run_adding_gain = setRunAddingGainHarmonicGen;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (harmonicGenDescriptor) {
		free((LADSPA_PortDescriptor *)harmonicGenDescriptor->PortDescriptors);
		free((char **)harmonicGenDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)harmonicGenDescriptor->PortRangeHints);
		free(harmonicGenDescriptor);
	}

}
