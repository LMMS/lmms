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

#line 10 "xfade_1915.xml"

#include "ladspa-util.h"

#define XFADE_XFADE                    0
#define XFADE_INPUTLA                  1
#define XFADE_INPUTRA                  2
#define XFADE_INPUTLB                  3
#define XFADE_INPUTRB                  4
#define XFADE_OUTPUTL                  5
#define XFADE_OUTPUTR                  6
#define XFADE4_XFADE                   0
#define XFADE4_INPUTLA                 1
#define XFADE4_INPUTRA                 2
#define XFADE4_INPUTLB                 3
#define XFADE4_INPUTRB                 4
#define XFADE4_OUTPUTLA                5
#define XFADE4_OUTPUTRA                6
#define XFADE4_OUTPUTLB                7
#define XFADE4_OUTPUTRB                8

static LADSPA_Descriptor *xfadeDescriptor = NULL;

typedef struct {
	LADSPA_Data *xfade;
	LADSPA_Data *inputLA;
	LADSPA_Data *inputRA;
	LADSPA_Data *inputLB;
	LADSPA_Data *inputRB;
	LADSPA_Data *outputL;
	LADSPA_Data *outputR;
	LADSPA_Data run_adding_gain;
} Xfade;

static LADSPA_Descriptor *xfade4Descriptor = NULL;

typedef struct {
	LADSPA_Data *xfade;
	LADSPA_Data *inputLA;
	LADSPA_Data *inputRA;
	LADSPA_Data *inputLB;
	LADSPA_Data *inputRB;
	LADSPA_Data *outputLA;
	LADSPA_Data *outputRA;
	LADSPA_Data *outputLB;
	LADSPA_Data *outputRB;
	LADSPA_Data run_adding_gain;
} Xfade4;

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
		return xfadeDescriptor;
	case 1:
		return xfade4Descriptor;
	default:
		return NULL;
	}
}

static void cleanupXfade(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortXfade(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Xfade *plugin;

	plugin = (Xfade *)instance;
	switch (port) {
	case XFADE_XFADE:
		plugin->xfade = data;
		break;
	case XFADE_INPUTLA:
		plugin->inputLA = data;
		break;
	case XFADE_INPUTRA:
		plugin->inputRA = data;
		break;
	case XFADE_INPUTLB:
		plugin->inputLB = data;
		break;
	case XFADE_INPUTRB:
		plugin->inputRB = data;
		break;
	case XFADE_OUTPUTL:
		plugin->outputL = data;
		break;
	case XFADE_OUTPUTR:
		plugin->outputR = data;
		break;
	}
}

static LADSPA_Handle instantiateXfade(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Xfade *plugin_data = (Xfade *)malloc(sizeof(Xfade));
	plugin_data->run_adding_gain = 1.0f;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runXfade(LADSPA_Handle instance, unsigned long sample_count) {
	Xfade *plugin_data = (Xfade *)instance;

	/* Crossfade (float value) */
	const LADSPA_Data xfade = *(plugin_data->xfade);

	/* Input A left (array of floats of length sample_count) */
	const LADSPA_Data * const inputLA = plugin_data->inputLA;

	/* Input A right (array of floats of length sample_count) */
	const LADSPA_Data * const inputRA = plugin_data->inputRA;

	/* Input B left (array of floats of length sample_count) */
	const LADSPA_Data * const inputLB = plugin_data->inputLB;

	/* Input B right (array of floats of length sample_count) */
	const LADSPA_Data * const inputRB = plugin_data->inputRB;

	/* Output left (array of floats of length sample_count) */
	LADSPA_Data * const outputL = plugin_data->outputL;

	/* Output right (array of floats of length sample_count) */
	LADSPA_Data * const outputR = plugin_data->outputR;

#line 19 "xfade_1915.xml"
	unsigned long pos;
	const float coefB = (xfade + 1.0f) * 0.5f;
	const float coefA = 1.0f - coefB;

	for (pos = 0; pos < sample_count; pos++) {
	  buffer_write(outputL[pos], inputLA[pos] * coefA + inputLB[pos] * coefB);
	  buffer_write(outputR[pos], inputRA[pos] * coefA + inputRB[pos] * coefB);
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainXfade(LADSPA_Handle instance, LADSPA_Data gain) {
	((Xfade *)instance)->run_adding_gain = gain;
}

static void runAddingXfade(LADSPA_Handle instance, unsigned long sample_count) {
	Xfade *plugin_data = (Xfade *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Crossfade (float value) */
	const LADSPA_Data xfade = *(plugin_data->xfade);

	/* Input A left (array of floats of length sample_count) */
	const LADSPA_Data * const inputLA = plugin_data->inputLA;

	/* Input A right (array of floats of length sample_count) */
	const LADSPA_Data * const inputRA = plugin_data->inputRA;

	/* Input B left (array of floats of length sample_count) */
	const LADSPA_Data * const inputLB = plugin_data->inputLB;

	/* Input B right (array of floats of length sample_count) */
	const LADSPA_Data * const inputRB = plugin_data->inputRB;

	/* Output left (array of floats of length sample_count) */
	LADSPA_Data * const outputL = plugin_data->outputL;

	/* Output right (array of floats of length sample_count) */
	LADSPA_Data * const outputR = plugin_data->outputR;

#line 19 "xfade_1915.xml"
	unsigned long pos;
	const float coefB = (xfade + 1.0f) * 0.5f;
	const float coefA = 1.0f - coefB;

	for (pos = 0; pos < sample_count; pos++) {
	  buffer_write(outputL[pos], inputLA[pos] * coefA + inputLB[pos] * coefB);
	  buffer_write(outputR[pos], inputRA[pos] * coefA + inputRB[pos] * coefB);
	}
}

static void cleanupXfade4(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortXfade4(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Xfade4 *plugin;

	plugin = (Xfade4 *)instance;
	switch (port) {
	case XFADE4_XFADE:
		plugin->xfade = data;
		break;
	case XFADE4_INPUTLA:
		plugin->inputLA = data;
		break;
	case XFADE4_INPUTRA:
		plugin->inputRA = data;
		break;
	case XFADE4_INPUTLB:
		plugin->inputLB = data;
		break;
	case XFADE4_INPUTRB:
		plugin->inputRB = data;
		break;
	case XFADE4_OUTPUTLA:
		plugin->outputLA = data;
		break;
	case XFADE4_OUTPUTRA:
		plugin->outputRA = data;
		break;
	case XFADE4_OUTPUTLB:
		plugin->outputLB = data;
		break;
	case XFADE4_OUTPUTRB:
		plugin->outputRB = data;
		break;
	}
}

static LADSPA_Handle instantiateXfade4(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Xfade4 *plugin_data = (Xfade4 *)malloc(sizeof(Xfade4));
	plugin_data->run_adding_gain = 1.0f;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runXfade4(LADSPA_Handle instance, unsigned long sample_count) {
	Xfade4 *plugin_data = (Xfade4 *)instance;

	/* Crossfade (float value) */
	const LADSPA_Data xfade = *(plugin_data->xfade);

	/* Input A left (array of floats of length sample_count) */
	const LADSPA_Data * const inputLA = plugin_data->inputLA;

	/* Input A right (array of floats of length sample_count) */
	const LADSPA_Data * const inputRA = plugin_data->inputRA;

	/* Input B left (array of floats of length sample_count) */
	const LADSPA_Data * const inputLB = plugin_data->inputLB;

	/* Input B right (array of floats of length sample_count) */
	const LADSPA_Data * const inputRB = plugin_data->inputRB;

	/* Output A left (array of floats of length sample_count) */
	LADSPA_Data * const outputLA = plugin_data->outputLA;

	/* Output A right (array of floats of length sample_count) */
	LADSPA_Data * const outputRA = plugin_data->outputRA;

	/* Output B left (array of floats of length sample_count) */
	LADSPA_Data * const outputLB = plugin_data->outputLB;

	/* Output B right (array of floats of length sample_count) */
	LADSPA_Data * const outputRB = plugin_data->outputRB;

#line 19 "xfade_1915.xml"
	unsigned long pos;
	const float coefB = (xfade + 1.0f) * 0.5f;
	const float coefA = 1.0f - coefB;

	for (pos = 0; pos < sample_count; pos++) {
	  buffer_write(outputLA[pos], inputLA[pos] * coefA);
	  buffer_write(outputRA[pos], inputRA[pos] * coefA);
	  buffer_write(outputLB[pos], inputLB[pos] * coefB);
	  buffer_write(outputRB[pos], inputRB[pos] * coefB);
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainXfade4(LADSPA_Handle instance, LADSPA_Data gain) {
	((Xfade4 *)instance)->run_adding_gain = gain;
}

static void runAddingXfade4(LADSPA_Handle instance, unsigned long sample_count) {
	Xfade4 *plugin_data = (Xfade4 *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Crossfade (float value) */
	const LADSPA_Data xfade = *(plugin_data->xfade);

	/* Input A left (array of floats of length sample_count) */
	const LADSPA_Data * const inputLA = plugin_data->inputLA;

	/* Input A right (array of floats of length sample_count) */
	const LADSPA_Data * const inputRA = plugin_data->inputRA;

	/* Input B left (array of floats of length sample_count) */
	const LADSPA_Data * const inputLB = plugin_data->inputLB;

	/* Input B right (array of floats of length sample_count) */
	const LADSPA_Data * const inputRB = plugin_data->inputRB;

	/* Output A left (array of floats of length sample_count) */
	LADSPA_Data * const outputLA = plugin_data->outputLA;

	/* Output A right (array of floats of length sample_count) */
	LADSPA_Data * const outputRA = plugin_data->outputRA;

	/* Output B left (array of floats of length sample_count) */
	LADSPA_Data * const outputLB = plugin_data->outputLB;

	/* Output B right (array of floats of length sample_count) */
	LADSPA_Data * const outputRB = plugin_data->outputRB;

#line 19 "xfade_1915.xml"
	unsigned long pos;
	const float coefB = (xfade + 1.0f) * 0.5f;
	const float coefA = 1.0f - coefB;

	for (pos = 0; pos < sample_count; pos++) {
	  buffer_write(outputLA[pos], inputLA[pos] * coefA);
	  buffer_write(outputRA[pos], inputRA[pos] * coefA);
	  buffer_write(outputLB[pos], inputLB[pos] * coefB);
	  buffer_write(outputRB[pos], inputRB[pos] * coefB);
	}
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


	xfadeDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (xfadeDescriptor) {
		xfadeDescriptor->UniqueID = 1915;
		xfadeDescriptor->Label = "xfade";
		xfadeDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		xfadeDescriptor->Name =
		 D_("Crossfade");
		xfadeDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		xfadeDescriptor->Copyright =
		 "GPL";
		xfadeDescriptor->PortCount = 7;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(7,
		 sizeof(LADSPA_PortDescriptor));
		xfadeDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(7,
		 sizeof(LADSPA_PortRangeHint));
		xfadeDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(7, sizeof(char*));
		xfadeDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Crossfade */
		port_descriptors[XFADE_XFADE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[XFADE_XFADE] =
		 D_("Crossfade");
		port_range_hints[XFADE_XFADE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[XFADE_XFADE].LowerBound = -1;
		port_range_hints[XFADE_XFADE].UpperBound = 1;

		/* Parameters for Input A left */
		port_descriptors[XFADE_INPUTLA] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[XFADE_INPUTLA] =
		 D_("Input A left");
		port_range_hints[XFADE_INPUTLA].HintDescriptor = 0;

		/* Parameters for Input A right */
		port_descriptors[XFADE_INPUTRA] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[XFADE_INPUTRA] =
		 D_("Input A right");
		port_range_hints[XFADE_INPUTRA].HintDescriptor = 0;

		/* Parameters for Input B left */
		port_descriptors[XFADE_INPUTLB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[XFADE_INPUTLB] =
		 D_("Input B left");
		port_range_hints[XFADE_INPUTLB].HintDescriptor = 0;

		/* Parameters for Input B right */
		port_descriptors[XFADE_INPUTRB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[XFADE_INPUTRB] =
		 D_("Input B right");
		port_range_hints[XFADE_INPUTRB].HintDescriptor = 0;

		/* Parameters for Output left */
		port_descriptors[XFADE_OUTPUTL] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[XFADE_OUTPUTL] =
		 D_("Output left");
		port_range_hints[XFADE_OUTPUTL].HintDescriptor = 0;

		/* Parameters for Output right */
		port_descriptors[XFADE_OUTPUTR] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[XFADE_OUTPUTR] =
		 D_("Output right");
		port_range_hints[XFADE_OUTPUTR].HintDescriptor = 0;

		xfadeDescriptor->activate = NULL;
		xfadeDescriptor->cleanup = cleanupXfade;
		xfadeDescriptor->connect_port = connectPortXfade;
		xfadeDescriptor->deactivate = NULL;
		xfadeDescriptor->instantiate = instantiateXfade;
		xfadeDescriptor->run = runXfade;
		xfadeDescriptor->run_adding = runAddingXfade;
		xfadeDescriptor->set_run_adding_gain = setRunAddingGainXfade;
	}

	xfade4Descriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (xfade4Descriptor) {
		xfade4Descriptor->UniqueID = 1917;
		xfade4Descriptor->Label = "xfade4";
		xfade4Descriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		xfade4Descriptor->Name =
		 D_("Crossfade (4 outs)");
		xfade4Descriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		xfade4Descriptor->Copyright =
		 "GPL";
		xfade4Descriptor->PortCount = 9;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(9,
		 sizeof(LADSPA_PortDescriptor));
		xfade4Descriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(9,
		 sizeof(LADSPA_PortRangeHint));
		xfade4Descriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(9, sizeof(char*));
		xfade4Descriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Crossfade */
		port_descriptors[XFADE4_XFADE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[XFADE4_XFADE] =
		 D_("Crossfade");
		port_range_hints[XFADE4_XFADE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[XFADE4_XFADE].LowerBound = -1;
		port_range_hints[XFADE4_XFADE].UpperBound = 1;

		/* Parameters for Input A left */
		port_descriptors[XFADE4_INPUTLA] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[XFADE4_INPUTLA] =
		 D_("Input A left");
		port_range_hints[XFADE4_INPUTLA].HintDescriptor = 0;

		/* Parameters for Input A right */
		port_descriptors[XFADE4_INPUTRA] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[XFADE4_INPUTRA] =
		 D_("Input A right");
		port_range_hints[XFADE4_INPUTRA].HintDescriptor = 0;

		/* Parameters for Input B left */
		port_descriptors[XFADE4_INPUTLB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[XFADE4_INPUTLB] =
		 D_("Input B left");
		port_range_hints[XFADE4_INPUTLB].HintDescriptor = 0;

		/* Parameters for Input B right */
		port_descriptors[XFADE4_INPUTRB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[XFADE4_INPUTRB] =
		 D_("Input B right");
		port_range_hints[XFADE4_INPUTRB].HintDescriptor = 0;

		/* Parameters for Output A left */
		port_descriptors[XFADE4_OUTPUTLA] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[XFADE4_OUTPUTLA] =
		 D_("Output A left");
		port_range_hints[XFADE4_OUTPUTLA].HintDescriptor = 0;

		/* Parameters for Output A right */
		port_descriptors[XFADE4_OUTPUTRA] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[XFADE4_OUTPUTRA] =
		 D_("Output A right");
		port_range_hints[XFADE4_OUTPUTRA].HintDescriptor = 0;

		/* Parameters for Output B left */
		port_descriptors[XFADE4_OUTPUTLB] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[XFADE4_OUTPUTLB] =
		 D_("Output B left");
		port_range_hints[XFADE4_OUTPUTLB].HintDescriptor = 0;

		/* Parameters for Output B right */
		port_descriptors[XFADE4_OUTPUTRB] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[XFADE4_OUTPUTRB] =
		 D_("Output B right");
		port_range_hints[XFADE4_OUTPUTRB].HintDescriptor = 0;

		xfade4Descriptor->activate = NULL;
		xfade4Descriptor->cleanup = cleanupXfade4;
		xfade4Descriptor->connect_port = connectPortXfade4;
		xfade4Descriptor->deactivate = NULL;
		xfade4Descriptor->instantiate = instantiateXfade4;
		xfade4Descriptor->run = runXfade4;
		xfade4Descriptor->run_adding = runAddingXfade4;
		xfade4Descriptor->set_run_adding_gain = setRunAddingGainXfade4;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (xfadeDescriptor) {
		free((LADSPA_PortDescriptor *)xfadeDescriptor->PortDescriptors);
		free((char **)xfadeDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)xfadeDescriptor->PortRangeHints);
		free(xfadeDescriptor);
	}
	if (xfade4Descriptor) {
		free((LADSPA_PortDescriptor *)xfade4Descriptor->PortDescriptors);
		free((char **)xfade4Descriptor->PortNames);
		free((LADSPA_PortRangeHint *)xfade4Descriptor->PortRangeHints);
		free(xfade4Descriptor);
	}

}
