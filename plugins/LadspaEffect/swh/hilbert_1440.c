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

#line 10 "hilbert_1440.xml"

#include "ladspa-util.h"

#define D_SIZE 256
#define NZEROS 200

/* The non-zero taps of the Hilbert transformer */
static float xcoeffs[] = {
     +0.0008103736f, +0.0008457886f, +0.0009017196f, +0.0009793364f,
     +0.0010798341f, +0.0012044365f, +0.0013544008f, +0.0015310235f,
     +0.0017356466f, +0.0019696659f, +0.0022345404f, +0.0025318040f,
     +0.0028630784f, +0.0032300896f, +0.0036346867f, +0.0040788644f,
     +0.0045647903f, +0.0050948365f, +0.0056716186f, +0.0062980419f,
     +0.0069773575f, +0.0077132300f, +0.0085098208f, +0.0093718901f,
     +0.0103049226f, +0.0113152847f, +0.0124104218f, +0.0135991079f,
     +0.0148917649f, +0.0163008758f, +0.0178415242f, +0.0195321089f,
     +0.0213953037f, +0.0234593652f, +0.0257599469f, +0.0283426636f,
     +0.0312667947f, +0.0346107648f, +0.0384804823f, +0.0430224431f,
     +0.0484451086f, +0.0550553725f, +0.0633242001f, +0.0740128560f,
     +0.0884368322f, +0.1090816773f, +0.1412745301f, +0.1988673273f,
     +0.3326528346f, +0.9997730178f, -0.9997730178f, -0.3326528346f,
     -0.1988673273f, -0.1412745301f, -0.1090816773f, -0.0884368322f,
     -0.0740128560f, -0.0633242001f, -0.0550553725f, -0.0484451086f,
     -0.0430224431f, -0.0384804823f, -0.0346107648f, -0.0312667947f,
     -0.0283426636f, -0.0257599469f, -0.0234593652f, -0.0213953037f,
     -0.0195321089f, -0.0178415242f, -0.0163008758f, -0.0148917649f,
     -0.0135991079f, -0.0124104218f, -0.0113152847f, -0.0103049226f,
     -0.0093718901f, -0.0085098208f, -0.0077132300f, -0.0069773575f,
     -0.0062980419f, -0.0056716186f, -0.0050948365f, -0.0045647903f,
     -0.0040788644f, -0.0036346867f, -0.0032300896f, -0.0028630784f,
     -0.0025318040f, -0.0022345404f, -0.0019696659f, -0.0017356466f,
     -0.0015310235f, -0.0013544008f, -0.0012044365f, -0.0010798341f,
     -0.0009793364f, -0.0009017196f, -0.0008457886f, -0.0008103736f,
};

#define HILBERT_INPUT                  0
#define HILBERT_OUTPUT0                1
#define HILBERT_OUTPUT90               2
#define HILBERT_LATENCY                3

static LADSPA_Descriptor *hilbertDescriptor = NULL;

typedef struct {
	LADSPA_Data *input;
	LADSPA_Data *output0;
	LADSPA_Data *output90;
	LADSPA_Data *latency;
	LADSPA_Data *delay;
	unsigned int dptr;
	LADSPA_Data run_adding_gain;
} Hilbert;

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
		return hilbertDescriptor;
	default:
		return NULL;
	}
}

static void cleanupHilbert(LADSPA_Handle instance) {
#line 59 "hilbert_1440.xml"
	Hilbert *plugin_data = (Hilbert *)instance;
	free(plugin_data->delay);
	free(instance);
}

static void connectPortHilbert(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Hilbert *plugin;

	plugin = (Hilbert *)instance;
	switch (port) {
	case HILBERT_INPUT:
		plugin->input = data;
		break;
	case HILBERT_OUTPUT0:
		plugin->output0 = data;
		break;
	case HILBERT_OUTPUT90:
		plugin->output90 = data;
		break;
	case HILBERT_LATENCY:
		plugin->latency = data;
		break;
	}
}

static LADSPA_Handle instantiateHilbert(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Hilbert *plugin_data = (Hilbert *)malloc(sizeof(Hilbert));
	LADSPA_Data *delay = NULL;
	unsigned int dptr;

#line 53 "hilbert_1440.xml"
	delay = calloc(D_SIZE, sizeof(LADSPA_Data));

	dptr = 0;

	plugin_data->delay = delay;
	plugin_data->dptr = dptr;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runHilbert(LADSPA_Handle instance, unsigned long sample_count) {
	Hilbert *plugin_data = (Hilbert *)instance;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* 0deg output (array of floats of length sample_count) */
	LADSPA_Data * const output0 = plugin_data->output0;

	/* 90deg output (array of floats of length sample_count) */
	LADSPA_Data * const output90 = plugin_data->output90;
	LADSPA_Data * delay = plugin_data->delay;
	unsigned int dptr = plugin_data->dptr;

#line 63 "hilbert_1440.xml"
	unsigned long pos;
	unsigned int i;
	float hilb;

	for (pos = 0; pos < sample_count; pos++) {
	  delay[dptr] = input[pos];
	  hilb = 0.0f;
	  for (i = 0; i < NZEROS/2; i++) {
	    hilb += (xcoeffs[i] * delay[(dptr - i*2) & (D_SIZE - 1)]);
	  }
	  buffer_write(output0[pos], delay[(dptr - 99) & (D_SIZE - 1)]);
	  buffer_write(output90[pos], hilb);
	  dptr = (dptr + 1) & (D_SIZE - 1);
	}

	plugin_data->dptr = dptr;

	*(plugin_data->latency) = 99;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainHilbert(LADSPA_Handle instance, LADSPA_Data gain) {
	((Hilbert *)instance)->run_adding_gain = gain;
}

static void runAddingHilbert(LADSPA_Handle instance, unsigned long sample_count) {
	Hilbert *plugin_data = (Hilbert *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* 0deg output (array of floats of length sample_count) */
	LADSPA_Data * const output0 = plugin_data->output0;

	/* 90deg output (array of floats of length sample_count) */
	LADSPA_Data * const output90 = plugin_data->output90;
	LADSPA_Data * delay = plugin_data->delay;
	unsigned int dptr = plugin_data->dptr;

#line 63 "hilbert_1440.xml"
	unsigned long pos;
	unsigned int i;
	float hilb;

	for (pos = 0; pos < sample_count; pos++) {
	  delay[dptr] = input[pos];
	  hilb = 0.0f;
	  for (i = 0; i < NZEROS/2; i++) {
	    hilb += (xcoeffs[i] * delay[(dptr - i*2) & (D_SIZE - 1)]);
	  }
	  buffer_write(output0[pos], delay[(dptr - 99) & (D_SIZE - 1)]);
	  buffer_write(output90[pos], hilb);
	  dptr = (dptr + 1) & (D_SIZE - 1);
	}

	plugin_data->dptr = dptr;

	*(plugin_data->latency) = 99;
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


	hilbertDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (hilbertDescriptor) {
		hilbertDescriptor->UniqueID = 1440;
		hilbertDescriptor->Label = "hilbert";
		hilbertDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		hilbertDescriptor->Name =
		 D_("Hilbert transformer");
		hilbertDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		hilbertDescriptor->Copyright =
		 "GPL";
		hilbertDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		hilbertDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		hilbertDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		hilbertDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Input */
		port_descriptors[HILBERT_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[HILBERT_INPUT] =
		 D_("Input");
		port_range_hints[HILBERT_INPUT].HintDescriptor = 0;

		/* Parameters for 0deg output */
		port_descriptors[HILBERT_OUTPUT0] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[HILBERT_OUTPUT0] =
		 D_("0deg output");
		port_range_hints[HILBERT_OUTPUT0].HintDescriptor = 0;

		/* Parameters for 90deg output */
		port_descriptors[HILBERT_OUTPUT90] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[HILBERT_OUTPUT90] =
		 D_("90deg output");
		port_range_hints[HILBERT_OUTPUT90].HintDescriptor = 0;

		/* Parameters for latency */
		port_descriptors[HILBERT_LATENCY] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
		port_names[HILBERT_LATENCY] =
		 D_("latency");
		port_range_hints[HILBERT_LATENCY].HintDescriptor = 0;

		hilbertDescriptor->activate = NULL;
		hilbertDescriptor->cleanup = cleanupHilbert;
		hilbertDescriptor->connect_port = connectPortHilbert;
		hilbertDescriptor->deactivate = NULL;
		hilbertDescriptor->instantiate = instantiateHilbert;
		hilbertDescriptor->run = runHilbert;
		hilbertDescriptor->run_adding = runAddingHilbert;
		hilbertDescriptor->set_run_adding_gain = setRunAddingGainHilbert;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (hilbertDescriptor) {
		free((LADSPA_PortDescriptor *)hilbertDescriptor->PortDescriptors);
		free((char **)hilbertDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)hilbertDescriptor->PortRangeHints);
		free(hilbertDescriptor);
	}

}
