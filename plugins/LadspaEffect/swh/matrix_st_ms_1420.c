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


#define MATRIXSTMS_LEFT                0
#define MATRIXSTMS_RIGHT               1
#define MATRIXSTMS_MID                 2
#define MATRIXSTMS_SIDE                3

static LADSPA_Descriptor *matrixStMSDescriptor = NULL;

typedef struct {
	LADSPA_Data *left;
	LADSPA_Data *right;
	LADSPA_Data *mid;
	LADSPA_Data *side;
	LADSPA_Data run_adding_gain;
} MatrixStMS;

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
		return matrixStMSDescriptor;
	default:
		return NULL;
	}
}

static void cleanupMatrixStMS(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortMatrixStMS(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	MatrixStMS *plugin;

	plugin = (MatrixStMS *)instance;
	switch (port) {
	case MATRIXSTMS_LEFT:
		plugin->left = data;
		break;
	case MATRIXSTMS_RIGHT:
		plugin->right = data;
		break;
	case MATRIXSTMS_MID:
		plugin->mid = data;
		break;
	case MATRIXSTMS_SIDE:
		plugin->side = data;
		break;
	}
}

static LADSPA_Handle instantiateMatrixStMS(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	MatrixStMS *plugin_data = (MatrixStMS *)malloc(sizeof(MatrixStMS));
	plugin_data->run_adding_gain = 1.0f;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runMatrixStMS(LADSPA_Handle instance, unsigned long sample_count) {
	MatrixStMS *plugin_data = (MatrixStMS *)instance;

	/* Left (array of floats of length sample_count) */
	const LADSPA_Data * const left = plugin_data->left;

	/* Right (array of floats of length sample_count) */
	const LADSPA_Data * const right = plugin_data->right;

	/* Mid (array of floats of length sample_count) */
	LADSPA_Data * const mid = plugin_data->mid;

	/* Side (array of floats of length sample_count) */
	LADSPA_Data * const side = plugin_data->side;

#line 16 "matrix_st_ms_1420.xml"
	unsigned long pos;

	for (pos = 0; pos < sample_count; pos++) {
	  buffer_write(mid[pos], (left[pos] + right[pos]) * 0.5);
	  buffer_write(side[pos], (left[pos] - right[pos]) * 0.5);
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainMatrixStMS(LADSPA_Handle instance, LADSPA_Data gain) {
	((MatrixStMS *)instance)->run_adding_gain = gain;
}

static void runAddingMatrixStMS(LADSPA_Handle instance, unsigned long sample_count) {
	MatrixStMS *plugin_data = (MatrixStMS *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Left (array of floats of length sample_count) */
	const LADSPA_Data * const left = plugin_data->left;

	/* Right (array of floats of length sample_count) */
	const LADSPA_Data * const right = plugin_data->right;

	/* Mid (array of floats of length sample_count) */
	LADSPA_Data * const mid = plugin_data->mid;

	/* Side (array of floats of length sample_count) */
	LADSPA_Data * const side = plugin_data->side;

#line 16 "matrix_st_ms_1420.xml"
	unsigned long pos;

	for (pos = 0; pos < sample_count; pos++) {
	  buffer_write(mid[pos], (left[pos] + right[pos]) * 0.5);
	  buffer_write(side[pos], (left[pos] - right[pos]) * 0.5);
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


	matrixStMSDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (matrixStMSDescriptor) {
		matrixStMSDescriptor->UniqueID = 1420;
		matrixStMSDescriptor->Label = "matrixStMS";
		matrixStMSDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		matrixStMSDescriptor->Name =
		 D_("Matrix: Stereo to MS");
		matrixStMSDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		matrixStMSDescriptor->Copyright =
		 "GPL";
		matrixStMSDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		matrixStMSDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		matrixStMSDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		matrixStMSDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Left */
		port_descriptors[MATRIXSTMS_LEFT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[MATRIXSTMS_LEFT] =
		 D_("Left");
		port_range_hints[MATRIXSTMS_LEFT].HintDescriptor = 0;

		/* Parameters for Right */
		port_descriptors[MATRIXSTMS_RIGHT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[MATRIXSTMS_RIGHT] =
		 D_("Right");
		port_range_hints[MATRIXSTMS_RIGHT].HintDescriptor = 0;

		/* Parameters for Mid */
		port_descriptors[MATRIXSTMS_MID] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[MATRIXSTMS_MID] =
		 D_("Mid");
		port_range_hints[MATRIXSTMS_MID].HintDescriptor = 0;

		/* Parameters for Side */
		port_descriptors[MATRIXSTMS_SIDE] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[MATRIXSTMS_SIDE] =
		 D_("Side");
		port_range_hints[MATRIXSTMS_SIDE].HintDescriptor = 0;

		matrixStMSDescriptor->activate = NULL;
		matrixStMSDescriptor->cleanup = cleanupMatrixStMS;
		matrixStMSDescriptor->connect_port = connectPortMatrixStMS;
		matrixStMSDescriptor->deactivate = NULL;
		matrixStMSDescriptor->instantiate = instantiateMatrixStMS;
		matrixStMSDescriptor->run = runMatrixStMS;
		matrixStMSDescriptor->run_adding = runAddingMatrixStMS;
		matrixStMSDescriptor->set_run_adding_gain = setRunAddingGainMatrixStMS;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (matrixStMSDescriptor) {
		free((LADSPA_PortDescriptor *)matrixStMSDescriptor->PortDescriptors);
		free((char **)matrixStMSDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)matrixStMSDescriptor->PortRangeHints);
		free(matrixStMSDescriptor);
	}

}
