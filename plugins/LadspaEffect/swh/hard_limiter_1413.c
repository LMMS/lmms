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

#line 10 "hard_limiter_1413.xml"

#include <math.h>
#include "ladspa-util.h"

#define HARDLIMITER_LIMIT_DB           0
#define HARDLIMITER_WET_GAIN           1
#define HARDLIMITER_RES_GAIN           2
#define HARDLIMITER_INPUT              3
#define HARDLIMITER_OUTPUT             4

static LADSPA_Descriptor *hardLimiterDescriptor = NULL;

typedef struct {
	LADSPA_Data *limit_db;
	LADSPA_Data *wet_gain;
	LADSPA_Data *res_gain;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data run_adding_gain;
} HardLimiter;

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
		return hardLimiterDescriptor;
	default:
		return NULL;
	}
}

static void cleanupHardLimiter(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortHardLimiter(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	HardLimiter *plugin;

	plugin = (HardLimiter *)instance;
	switch (port) {
	case HARDLIMITER_LIMIT_DB:
		plugin->limit_db = data;
		break;
	case HARDLIMITER_WET_GAIN:
		plugin->wet_gain = data;
		break;
	case HARDLIMITER_RES_GAIN:
		plugin->res_gain = data;
		break;
	case HARDLIMITER_INPUT:
		plugin->input = data;
		break;
	case HARDLIMITER_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateHardLimiter(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	HardLimiter *plugin_data = (HardLimiter *)malloc(sizeof(HardLimiter));
	plugin_data->run_adding_gain = 1.0f;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runHardLimiter(LADSPA_Handle instance, unsigned long sample_count) {
	HardLimiter *plugin_data = (HardLimiter *)instance;

	/* dB limit (float value) */
	const LADSPA_Data limit_db = *(plugin_data->limit_db);

	/* Wet level (float value) */
	const LADSPA_Data wet_gain = *(plugin_data->wet_gain);

	/* Residue level (float value) */
	const LADSPA_Data res_gain = *(plugin_data->res_gain);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

#line 21 "hard_limiter_1413.xml"
	unsigned long i;
	for (i = 0; i < sample_count; i++)
	{
	        float limit_g = pow(10, limit_db / 20);
	        float sign = input[i] < 0.0 ? -1.0 : 1.0;
	        float data = input[i] * sign;
	        float residue = data > limit_g ? data - limit_g : 0.0;
	        data -= residue;
	        buffer_write(output[i],
	                 sign * (wet_gain * data + res_gain * residue));
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainHardLimiter(LADSPA_Handle instance, LADSPA_Data gain) {
	((HardLimiter *)instance)->run_adding_gain = gain;
}

static void runAddingHardLimiter(LADSPA_Handle instance, unsigned long sample_count) {
	HardLimiter *plugin_data = (HardLimiter *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* dB limit (float value) */
	const LADSPA_Data limit_db = *(plugin_data->limit_db);

	/* Wet level (float value) */
	const LADSPA_Data wet_gain = *(plugin_data->wet_gain);

	/* Residue level (float value) */
	const LADSPA_Data res_gain = *(plugin_data->res_gain);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

#line 21 "hard_limiter_1413.xml"
	unsigned long i;
	for (i = 0; i < sample_count; i++)
	{
	        float limit_g = pow(10, limit_db / 20);
	        float sign = input[i] < 0.0 ? -1.0 : 1.0;
	        float data = input[i] * sign;
	        float residue = data > limit_g ? data - limit_g : 0.0;
	        data -= residue;
	        buffer_write(output[i],
	                 sign * (wet_gain * data + res_gain * residue));
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


	hardLimiterDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (hardLimiterDescriptor) {
		hardLimiterDescriptor->UniqueID = 1413;
		hardLimiterDescriptor->Label = "hardLimiter";
		hardLimiterDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		hardLimiterDescriptor->Name =
		 D_("Hard Limiter");
		hardLimiterDescriptor->Maker =
		 "Marcus Andersson";
		hardLimiterDescriptor->Copyright =
		 "GPL";
		hardLimiterDescriptor->PortCount = 5;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(5,
		 sizeof(LADSPA_PortDescriptor));
		hardLimiterDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(5,
		 sizeof(LADSPA_PortRangeHint));
		hardLimiterDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(5, sizeof(char*));
		hardLimiterDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for dB limit */
		port_descriptors[HARDLIMITER_LIMIT_DB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HARDLIMITER_LIMIT_DB] =
		 D_("dB limit");
		port_range_hints[HARDLIMITER_LIMIT_DB].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HARDLIMITER_LIMIT_DB].LowerBound = -50.0;
		port_range_hints[HARDLIMITER_LIMIT_DB].UpperBound = 0.0;

		/* Parameters for Wet level */
		port_descriptors[HARDLIMITER_WET_GAIN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HARDLIMITER_WET_GAIN] =
		 D_("Wet level");
		port_range_hints[HARDLIMITER_WET_GAIN].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[HARDLIMITER_WET_GAIN].LowerBound = 0.0;
		port_range_hints[HARDLIMITER_WET_GAIN].UpperBound = 1.0;

		/* Parameters for Residue level */
		port_descriptors[HARDLIMITER_RES_GAIN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HARDLIMITER_RES_GAIN] =
		 D_("Residue level");
		port_range_hints[HARDLIMITER_RES_GAIN].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HARDLIMITER_RES_GAIN].LowerBound = 0.0;
		port_range_hints[HARDLIMITER_RES_GAIN].UpperBound = 1.0;

		/* Parameters for Input */
		port_descriptors[HARDLIMITER_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[HARDLIMITER_INPUT] =
		 D_("Input");
		port_range_hints[HARDLIMITER_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[HARDLIMITER_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[HARDLIMITER_OUTPUT] =
		 D_("Output");
		port_range_hints[HARDLIMITER_OUTPUT].HintDescriptor = 0;

		hardLimiterDescriptor->activate = NULL;
		hardLimiterDescriptor->cleanup = cleanupHardLimiter;
		hardLimiterDescriptor->connect_port = connectPortHardLimiter;
		hardLimiterDescriptor->deactivate = NULL;
		hardLimiterDescriptor->instantiate = instantiateHardLimiter;
		hardLimiterDescriptor->run = runHardLimiter;
		hardLimiterDescriptor->run_adding = runAddingHardLimiter;
		hardLimiterDescriptor->set_run_adding_gain = setRunAddingGainHardLimiter;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (hardLimiterDescriptor) {
		free((LADSPA_PortDescriptor *)hardLimiterDescriptor->PortDescriptors);
		free((char **)hardLimiterDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)hardLimiterDescriptor->PortRangeHints);
		free(hardLimiterDescriptor);
	}

}
