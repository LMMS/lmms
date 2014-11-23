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


#define CONST_AMPLITUDE                0
#define CONST_INPUT                    1
#define CONST_OUTPUT                   2

static LADSPA_Descriptor *constDescriptor = NULL;

typedef struct {
	LADSPA_Data *amplitude;
	LADSPA_Data *input;
	LADSPA_Data *output;
	float        last_amp;
	LADSPA_Data run_adding_gain;
} Const;

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
		return constDescriptor;
	default:
		return NULL;
	}
}

static void activateConst(LADSPA_Handle instance) {
	Const *plugin_data = (Const *)instance;
	float last_amp = plugin_data->last_amp;
#line 18 "const_1909.xml"
	last_amp = 0.0f;
	plugin_data->last_amp = last_amp;

}

static void cleanupConst(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortConst(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Const *plugin;

	plugin = (Const *)instance;
	switch (port) {
	case CONST_AMPLITUDE:
		plugin->amplitude = data;
		break;
	case CONST_INPUT:
		plugin->input = data;
		break;
	case CONST_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateConst(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Const *plugin_data = (Const *)malloc(sizeof(Const));
	plugin_data->run_adding_gain = 1.0f;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runConst(LADSPA_Handle instance, unsigned long sample_count) {
	Const *plugin_data = (Const *)instance;

	/* Signal amplitude (float value) */
	const LADSPA_Data amplitude = *(plugin_data->amplitude);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	float last_amp = plugin_data->last_amp;

#line 22 "const_1909.xml"
	unsigned long pos;
	const float delta = (amplitude - last_amp) / (sample_count - 1);
	float amp = last_amp;

	for (pos = 0; pos < sample_count; pos++) {
	  amp += delta;
	  buffer_write(output[pos], input[pos] + amp);
	}

	plugin_data->last_amp = amp;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainConst(LADSPA_Handle instance, LADSPA_Data gain) {
	((Const *)instance)->run_adding_gain = gain;
}

static void runAddingConst(LADSPA_Handle instance, unsigned long sample_count) {
	Const *plugin_data = (Const *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Signal amplitude (float value) */
	const LADSPA_Data amplitude = *(plugin_data->amplitude);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	float last_amp = plugin_data->last_amp;

#line 22 "const_1909.xml"
	unsigned long pos;
	const float delta = (amplitude - last_amp) / (sample_count - 1);
	float amp = last_amp;

	for (pos = 0; pos < sample_count; pos++) {
	  amp += delta;
	  buffer_write(output[pos], input[pos] + amp);
	}

	plugin_data->last_amp = amp;
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


	constDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (constDescriptor) {
		constDescriptor->UniqueID = 1909;
		constDescriptor->Label = "const";
		constDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		constDescriptor->Name =
		 D_("Constant Signal Generator");
		constDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		constDescriptor->Copyright =
		 "GPL";
		constDescriptor->PortCount = 3;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(3,
		 sizeof(LADSPA_PortDescriptor));
		constDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(3,
		 sizeof(LADSPA_PortRangeHint));
		constDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(3, sizeof(char*));
		constDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Signal amplitude */
		port_descriptors[CONST_AMPLITUDE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[CONST_AMPLITUDE] =
		 D_("Signal amplitude");
		port_range_hints[CONST_AMPLITUDE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[CONST_AMPLITUDE].LowerBound = -1;
		port_range_hints[CONST_AMPLITUDE].UpperBound = 1.1;

		/* Parameters for Input */
		port_descriptors[CONST_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[CONST_INPUT] =
		 D_("Input");
		port_range_hints[CONST_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[CONST_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[CONST_OUTPUT] =
		 D_("Output");
		port_range_hints[CONST_OUTPUT].HintDescriptor = 0;

		constDescriptor->activate = activateConst;
		constDescriptor->cleanup = cleanupConst;
		constDescriptor->connect_port = connectPortConst;
		constDescriptor->deactivate = NULL;
		constDescriptor->instantiate = instantiateConst;
		constDescriptor->run = runConst;
		constDescriptor->run_adding = runAddingConst;
		constDescriptor->set_run_adding_gain = setRunAddingGainConst;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (constDescriptor) {
		free((LADSPA_PortDescriptor *)constDescriptor->PortDescriptors);
		free((char **)constDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)constDescriptor->PortRangeHints);
		free(constDescriptor);
	}

}
