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

#line 9 "declip_1195.xml"

#define MAX_AMP 1.0f
#define CLIP 0.8f
#define CLIP_A ((MAX_AMP - CLIP) * (MAX_AMP - CLIP))
#define CLIP_B (MAX_AMP - 2.0f * CLIP)

#define DECLIP_INPUT                   0
#define DECLIP_OUTPUT                  1

static LADSPA_Descriptor *declipDescriptor = NULL;

typedef struct {
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data run_adding_gain;
} Declip;

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
		return declipDescriptor;
	default:
		return NULL;
	}
}

static void cleanupDeclip(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortDeclip(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Declip *plugin;

	plugin = (Declip *)instance;
	switch (port) {
	case DECLIP_INPUT:
		plugin->input = data;
		break;
	case DECLIP_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateDeclip(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Declip *plugin_data = (Declip *)malloc(sizeof(Declip));
	plugin_data->run_adding_gain = 1.0f;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runDeclip(LADSPA_Handle instance, unsigned long sample_count) {
	Declip *plugin_data = (Declip *)instance;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

#line 23 "declip_1195.xml"
	unsigned long pos;

	for (pos = 0; pos < sample_count; pos++) {
	        const LADSPA_Data in = input[pos];

	        if((in < CLIP) && (in > -CLIP)) {
	                buffer_write(output[pos], in);
	        } else if (in > 0.0f) {
	                buffer_write(output[pos], MAX_AMP - (CLIP_A / (CLIP_B + in)));
	        } else {
	                buffer_write(output[pos], -(MAX_AMP - (CLIP_A / (CLIP_B - in))));
	        }
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainDeclip(LADSPA_Handle instance, LADSPA_Data gain) {
	((Declip *)instance)->run_adding_gain = gain;
}

static void runAddingDeclip(LADSPA_Handle instance, unsigned long sample_count) {
	Declip *plugin_data = (Declip *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

#line 23 "declip_1195.xml"
	unsigned long pos;

	for (pos = 0; pos < sample_count; pos++) {
	        const LADSPA_Data in = input[pos];

	        if((in < CLIP) && (in > -CLIP)) {
	                buffer_write(output[pos], in);
	        } else if (in > 0.0f) {
	                buffer_write(output[pos], MAX_AMP - (CLIP_A / (CLIP_B + in)));
	        } else {
	                buffer_write(output[pos], -(MAX_AMP - (CLIP_A / (CLIP_B - in))));
	        }
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


	declipDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (declipDescriptor) {
		declipDescriptor->UniqueID = 1195;
		declipDescriptor->Label = "declip";
		declipDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		declipDescriptor->Name =
		 D_("Declipper");
		declipDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		declipDescriptor->Copyright =
		 "GPL";
		declipDescriptor->PortCount = 2;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(2,
		 sizeof(LADSPA_PortDescriptor));
		declipDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(2,
		 sizeof(LADSPA_PortRangeHint));
		declipDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(2, sizeof(char*));
		declipDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Input */
		port_descriptors[DECLIP_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[DECLIP_INPUT] =
		 D_("Input");
		port_range_hints[DECLIP_INPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[DECLIP_INPUT].LowerBound = -1;
		port_range_hints[DECLIP_INPUT].UpperBound = +1;

		/* Parameters for Output */
		port_descriptors[DECLIP_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[DECLIP_OUTPUT] =
		 D_("Output");
		port_range_hints[DECLIP_OUTPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[DECLIP_OUTPUT].LowerBound = -1;
		port_range_hints[DECLIP_OUTPUT].UpperBound = +1;

		declipDescriptor->activate = NULL;
		declipDescriptor->cleanup = cleanupDeclip;
		declipDescriptor->connect_port = connectPortDeclip;
		declipDescriptor->deactivate = NULL;
		declipDescriptor->instantiate = instantiateDeclip;
		declipDescriptor->run = runDeclip;
		declipDescriptor->run_adding = runAddingDeclip;
		declipDescriptor->set_run_adding_gain = setRunAddingGainDeclip;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (declipDescriptor) {
		free((LADSPA_PortDescriptor *)declipDescriptor->PortDescriptors);
		free((char **)declipDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)declipDescriptor->PortRangeHints);
		free(declipDescriptor);
	}

}
