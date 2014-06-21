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


#define MATRIXMSST_WIDTH               0
#define MATRIXMSST_MID                 1
#define MATRIXMSST_SIDE                2
#define MATRIXMSST_LEFT                3
#define MATRIXMSST_RIGHT               4

static LADSPA_Descriptor *matrixMSStDescriptor = NULL;

typedef struct {
	LADSPA_Data *width;
	LADSPA_Data *mid;
	LADSPA_Data *side;
	LADSPA_Data *left;
	LADSPA_Data *right;
	LADSPA_Data run_adding_gain;
} MatrixMSSt;

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
		return matrixMSStDescriptor;
	default:
		return NULL;
	}
}

static void cleanupMatrixMSSt(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortMatrixMSSt(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	MatrixMSSt *plugin;

	plugin = (MatrixMSSt *)instance;
	switch (port) {
	case MATRIXMSST_WIDTH:
		plugin->width = data;
		break;
	case MATRIXMSST_MID:
		plugin->mid = data;
		break;
	case MATRIXMSST_SIDE:
		plugin->side = data;
		break;
	case MATRIXMSST_LEFT:
		plugin->left = data;
		break;
	case MATRIXMSST_RIGHT:
		plugin->right = data;
		break;
	}
}

static LADSPA_Handle instantiateMatrixMSSt(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	MatrixMSSt *plugin_data = (MatrixMSSt *)malloc(sizeof(MatrixMSSt));
	plugin_data->run_adding_gain = 1.0f;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runMatrixMSSt(LADSPA_Handle instance, unsigned long sample_count) {
	MatrixMSSt *plugin_data = (MatrixMSSt *)instance;

	/* Width (float value) */
	const LADSPA_Data width = *(plugin_data->width);

	/* Mid (array of floats of length sample_count) */
	const LADSPA_Data * const mid = plugin_data->mid;

	/* Side (array of floats of length sample_count) */
	const LADSPA_Data * const side = plugin_data->side;

	/* Left (array of floats of length sample_count) */
	LADSPA_Data * const left = plugin_data->left;

	/* Right (array of floats of length sample_count) */
	LADSPA_Data * const right = plugin_data->right;

#line 16 "matrix_ms_st_1421.xml"
	unsigned long pos;

	for (pos = 0; pos < sample_count; pos++) {
	  buffer_write(left[pos], mid[pos] + side[pos] * width);
	  buffer_write(right[pos], mid[pos] - side[pos] * width);
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainMatrixMSSt(LADSPA_Handle instance, LADSPA_Data gain) {
	((MatrixMSSt *)instance)->run_adding_gain = gain;
}

static void runAddingMatrixMSSt(LADSPA_Handle instance, unsigned long sample_count) {
	MatrixMSSt *plugin_data = (MatrixMSSt *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Width (float value) */
	const LADSPA_Data width = *(plugin_data->width);

	/* Mid (array of floats of length sample_count) */
	const LADSPA_Data * const mid = plugin_data->mid;

	/* Side (array of floats of length sample_count) */
	const LADSPA_Data * const side = plugin_data->side;

	/* Left (array of floats of length sample_count) */
	LADSPA_Data * const left = plugin_data->left;

	/* Right (array of floats of length sample_count) */
	LADSPA_Data * const right = plugin_data->right;

#line 16 "matrix_ms_st_1421.xml"
	unsigned long pos;

	for (pos = 0; pos < sample_count; pos++) {
	  buffer_write(left[pos], mid[pos] + side[pos] * width);
	  buffer_write(right[pos], mid[pos] - side[pos] * width);
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


	matrixMSStDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (matrixMSStDescriptor) {
		matrixMSStDescriptor->UniqueID = 1421;
		matrixMSStDescriptor->Label = "matrixMSSt";
		matrixMSStDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		matrixMSStDescriptor->Name =
		 D_("Matrix: MS to Stereo");
		matrixMSStDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		matrixMSStDescriptor->Copyright =
		 "GPL";
		matrixMSStDescriptor->PortCount = 5;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(5,
		 sizeof(LADSPA_PortDescriptor));
		matrixMSStDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(5,
		 sizeof(LADSPA_PortRangeHint));
		matrixMSStDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(5, sizeof(char*));
		matrixMSStDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Width */
		port_descriptors[MATRIXMSST_WIDTH] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[MATRIXMSST_WIDTH] =
		 D_("Width");
		port_range_hints[MATRIXMSST_WIDTH].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[MATRIXMSST_WIDTH].LowerBound = 0;
		port_range_hints[MATRIXMSST_WIDTH].UpperBound = 2;

		/* Parameters for Mid */
		port_descriptors[MATRIXMSST_MID] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[MATRIXMSST_MID] =
		 D_("Mid");
		port_range_hints[MATRIXMSST_MID].HintDescriptor = 0;

		/* Parameters for Side */
		port_descriptors[MATRIXMSST_SIDE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[MATRIXMSST_SIDE] =
		 D_("Side");
		port_range_hints[MATRIXMSST_SIDE].HintDescriptor = 0;

		/* Parameters for Left */
		port_descriptors[MATRIXMSST_LEFT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[MATRIXMSST_LEFT] =
		 D_("Left");
		port_range_hints[MATRIXMSST_LEFT].HintDescriptor = 0;

		/* Parameters for Right */
		port_descriptors[MATRIXMSST_RIGHT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[MATRIXMSST_RIGHT] =
		 D_("Right");
		port_range_hints[MATRIXMSST_RIGHT].HintDescriptor = 0;

		matrixMSStDescriptor->activate = NULL;
		matrixMSStDescriptor->cleanup = cleanupMatrixMSSt;
		matrixMSStDescriptor->connect_port = connectPortMatrixMSSt;
		matrixMSStDescriptor->deactivate = NULL;
		matrixMSStDescriptor->instantiate = instantiateMatrixMSSt;
		matrixMSStDescriptor->run = runMatrixMSSt;
		matrixMSStDescriptor->run_adding = runAddingMatrixMSSt;
		matrixMSStDescriptor->set_run_adding_gain = setRunAddingGainMatrixMSSt;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (matrixMSStDescriptor) {
		free((LADSPA_PortDescriptor *)matrixMSStDescriptor->PortDescriptors);
		free((char **)matrixMSStDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)matrixMSStDescriptor->PortRangeHints);
		free(matrixMSStDescriptor);
	}

}
