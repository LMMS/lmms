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

#line 10 "amp_1181.xml"

#include "ladspa-util.h"

#define AMP_GAIN                       0
#define AMP_INPUT                      1
#define AMP_OUTPUT                     2

static LADSPA_Descriptor *ampDescriptor = NULL;

typedef struct {
	LADSPA_Data *gain;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data run_adding_gain;
} Amp;

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
		return ampDescriptor;
	default:
		return NULL;
	}
}

static void cleanupAmp(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortAmp(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Amp *plugin;

	plugin = (Amp *)instance;
	switch (port) {
	case AMP_GAIN:
		plugin->gain = data;
		break;
	case AMP_INPUT:
		plugin->input = data;
		break;
	case AMP_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateAmp(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Amp *plugin_data = (Amp *)malloc(sizeof(Amp));
	plugin_data->run_adding_gain = 1.0f;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runAmp(LADSPA_Handle instance, unsigned long sample_count) {
	Amp *plugin_data = (Amp *)instance;

	/* Amps gain (dB) (float value) */
	const LADSPA_Data gain = *(plugin_data->gain);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

#line 19 "amp_1181.xml"
	unsigned long pos;
	float coef = DB_CO(gain);

	for (pos = 0; pos < sample_count; pos++) {
	  buffer_write(output[pos], input[pos] * coef);
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainAmp(LADSPA_Handle instance, LADSPA_Data gain) {
	((Amp *)instance)->run_adding_gain = gain;
}

static void runAddingAmp(LADSPA_Handle instance, unsigned long sample_count) {
	Amp *plugin_data = (Amp *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Amps gain (dB) (float value) */
	const LADSPA_Data gain = *(plugin_data->gain);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

#line 19 "amp_1181.xml"
	unsigned long pos;
	float coef = DB_CO(gain);

	for (pos = 0; pos < sample_count; pos++) {
	  buffer_write(output[pos], input[pos] * coef);
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


	ampDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (ampDescriptor) {
		ampDescriptor->UniqueID = 1181;
		ampDescriptor->Label = "amp";
		ampDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		ampDescriptor->Name =
		 D_("Simple amplifier");
		ampDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		ampDescriptor->Copyright =
		 "GPL";
		ampDescriptor->PortCount = 3;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(3,
		 sizeof(LADSPA_PortDescriptor));
		ampDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(3,
		 sizeof(LADSPA_PortRangeHint));
		ampDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(3, sizeof(char*));
		ampDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Amps gain (dB) */
		port_descriptors[AMP_GAIN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[AMP_GAIN] =
		 D_("Amps gain (dB)");
		port_range_hints[AMP_GAIN].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[AMP_GAIN].LowerBound = -70;
		port_range_hints[AMP_GAIN].UpperBound = +70;

		/* Parameters for Input */
		port_descriptors[AMP_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[AMP_INPUT] =
		 D_("Input");
		port_range_hints[AMP_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[AMP_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[AMP_OUTPUT] =
		 D_("Output");
		port_range_hints[AMP_OUTPUT].HintDescriptor = 0;

		ampDescriptor->activate = NULL;
		ampDescriptor->cleanup = cleanupAmp;
		ampDescriptor->connect_port = connectPortAmp;
		ampDescriptor->deactivate = NULL;
		ampDescriptor->instantiate = instantiateAmp;
		ampDescriptor->run = runAmp;
		ampDescriptor->run_adding = runAddingAmp;
		ampDescriptor->set_run_adding_gain = setRunAddingGainAmp;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (ampDescriptor) {
		free((LADSPA_PortDescriptor *)ampDescriptor->PortDescriptors);
		free((char **)ampDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)ampDescriptor->PortRangeHints);
		free(ampDescriptor);
	}

}
