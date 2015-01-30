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


#define FOVERDRIVE_DRIVE               0
#define FOVERDRIVE_INPUT               1
#define FOVERDRIVE_OUTPUT              2

static LADSPA_Descriptor *foverdriveDescriptor = NULL;

typedef struct {
	LADSPA_Data *drive;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data run_adding_gain;
} Foverdrive;

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
		return foverdriveDescriptor;
	default:
		return NULL;
	}
}

static void cleanupFoverdrive(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortFoverdrive(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Foverdrive *plugin;

	plugin = (Foverdrive *)instance;
	switch (port) {
	case FOVERDRIVE_DRIVE:
		plugin->drive = data;
		break;
	case FOVERDRIVE_INPUT:
		plugin->input = data;
		break;
	case FOVERDRIVE_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateFoverdrive(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Foverdrive *plugin_data = (Foverdrive *)malloc(sizeof(Foverdrive));
	plugin_data->run_adding_gain = 1.0f;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runFoverdrive(LADSPA_Handle instance, unsigned long sample_count) {
	Foverdrive *plugin_data = (Foverdrive *)instance;

	/* Drive level (float value) */
	const LADSPA_Data drive = *(plugin_data->drive);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

#line 16 "foverdrive_1196.xml"
	unsigned long pos;
	const float drivem1 = drive - 1.0f;

	for (pos = 0; pos < sample_count; pos++) {
	        LADSPA_Data x = input[pos];
	        const float fx = fabs(x);
	        buffer_write(output[pos], x*(fx + drive)/(x*x + drivem1*fx + 1.0f));
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainFoverdrive(LADSPA_Handle instance, LADSPA_Data gain) {
	((Foverdrive *)instance)->run_adding_gain = gain;
}

static void runAddingFoverdrive(LADSPA_Handle instance, unsigned long sample_count) {
	Foverdrive *plugin_data = (Foverdrive *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Drive level (float value) */
	const LADSPA_Data drive = *(plugin_data->drive);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

#line 16 "foverdrive_1196.xml"
	unsigned long pos;
	const float drivem1 = drive - 1.0f;

	for (pos = 0; pos < sample_count; pos++) {
	        LADSPA_Data x = input[pos];
	        const float fx = fabs(x);
	        buffer_write(output[pos], x*(fx + drive)/(x*x + drivem1*fx + 1.0f));
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


	foverdriveDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (foverdriveDescriptor) {
		foverdriveDescriptor->UniqueID = 1196;
		foverdriveDescriptor->Label = "foverdrive";
		foverdriveDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		foverdriveDescriptor->Name =
		 D_("Fast overdrive");
		foverdriveDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		foverdriveDescriptor->Copyright =
		 "GPL";
		foverdriveDescriptor->PortCount = 3;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(3,
		 sizeof(LADSPA_PortDescriptor));
		foverdriveDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(3,
		 sizeof(LADSPA_PortRangeHint));
		foverdriveDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(3, sizeof(char*));
		foverdriveDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Drive level */
		port_descriptors[FOVERDRIVE_DRIVE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[FOVERDRIVE_DRIVE] =
		 D_("Drive level");
		port_range_hints[FOVERDRIVE_DRIVE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MINIMUM;
		port_range_hints[FOVERDRIVE_DRIVE].LowerBound = 1;
		port_range_hints[FOVERDRIVE_DRIVE].UpperBound = 3;

		/* Parameters for Input */
		port_descriptors[FOVERDRIVE_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[FOVERDRIVE_INPUT] =
		 D_("Input");
		port_range_hints[FOVERDRIVE_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[FOVERDRIVE_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[FOVERDRIVE_OUTPUT] =
		 D_("Output");
		port_range_hints[FOVERDRIVE_OUTPUT].HintDescriptor = 0;

		foverdriveDescriptor->activate = NULL;
		foverdriveDescriptor->cleanup = cleanupFoverdrive;
		foverdriveDescriptor->connect_port = connectPortFoverdrive;
		foverdriveDescriptor->deactivate = NULL;
		foverdriveDescriptor->instantiate = instantiateFoverdrive;
		foverdriveDescriptor->run = runFoverdrive;
		foverdriveDescriptor->run_adding = runAddingFoverdrive;
		foverdriveDescriptor->set_run_adding_gain = setRunAddingGainFoverdrive;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (foverdriveDescriptor) {
		free((LADSPA_PortDescriptor *)foverdriveDescriptor->PortDescriptors);
		free((char **)foverdriveDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)foverdriveDescriptor->PortRangeHints);
		free(foverdriveDescriptor);
	}

}
