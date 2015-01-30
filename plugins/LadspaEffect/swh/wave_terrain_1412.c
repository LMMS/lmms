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


#define WAVETERRAIN_XB                 0
#define WAVETERRAIN_YB                 1
#define WAVETERRAIN_ZB                 2

static LADSPA_Descriptor *waveTerrainDescriptor = NULL;

typedef struct {
	LADSPA_Data *xb;
	LADSPA_Data *yb;
	LADSPA_Data *zb;
	LADSPA_Data run_adding_gain;
} WaveTerrain;

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
		return waveTerrainDescriptor;
	default:
		return NULL;
	}
}

static void cleanupWaveTerrain(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortWaveTerrain(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	WaveTerrain *plugin;

	plugin = (WaveTerrain *)instance;
	switch (port) {
	case WAVETERRAIN_XB:
		plugin->xb = data;
		break;
	case WAVETERRAIN_YB:
		plugin->yb = data;
		break;
	case WAVETERRAIN_ZB:
		plugin->zb = data;
		break;
	}
}

static LADSPA_Handle instantiateWaveTerrain(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	WaveTerrain *plugin_data = (WaveTerrain *)malloc(sizeof(WaveTerrain));
	plugin_data->run_adding_gain = 1.0f;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runWaveTerrain(LADSPA_Handle instance, unsigned long sample_count) {
	WaveTerrain *plugin_data = (WaveTerrain *)instance;

	/* x (array of floats of length sample_count) */
	const LADSPA_Data * const xb = plugin_data->xb;

	/* y (array of floats of length sample_count) */
	const LADSPA_Data * const yb = plugin_data->yb;

	/* z (array of floats of length sample_count) */
	LADSPA_Data * const zb = plugin_data->zb;

#line 18 "wave_terrain_1412.xml"
	unsigned long pos;
	float x, y;

	for (pos = 0; pos < sample_count; pos++) {
	  x = xb[pos];
	  y = yb[pos];
	  buffer_write(zb[pos], (x - y) * (x - 1.0f) * (x + 1.0f) * (y - 1.0f) * (y + 1.0f) );
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainWaveTerrain(LADSPA_Handle instance, LADSPA_Data gain) {
	((WaveTerrain *)instance)->run_adding_gain = gain;
}

static void runAddingWaveTerrain(LADSPA_Handle instance, unsigned long sample_count) {
	WaveTerrain *plugin_data = (WaveTerrain *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* x (array of floats of length sample_count) */
	const LADSPA_Data * const xb = plugin_data->xb;

	/* y (array of floats of length sample_count) */
	const LADSPA_Data * const yb = plugin_data->yb;

	/* z (array of floats of length sample_count) */
	LADSPA_Data * const zb = plugin_data->zb;

#line 18 "wave_terrain_1412.xml"
	unsigned long pos;
	float x, y;

	for (pos = 0; pos < sample_count; pos++) {
	  x = xb[pos];
	  y = yb[pos];
	  buffer_write(zb[pos], (x - y) * (x - 1.0f) * (x + 1.0f) * (y - 1.0f) * (y + 1.0f) );
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


	waveTerrainDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (waveTerrainDescriptor) {
		waveTerrainDescriptor->UniqueID = 1412;
		waveTerrainDescriptor->Label = "waveTerrain";
		waveTerrainDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		waveTerrainDescriptor->Name =
		 D_("Wave Terrain Oscillator");
		waveTerrainDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		waveTerrainDescriptor->Copyright =
		 "GPL";
		waveTerrainDescriptor->PortCount = 3;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(3,
		 sizeof(LADSPA_PortDescriptor));
		waveTerrainDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(3,
		 sizeof(LADSPA_PortRangeHint));
		waveTerrainDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(3, sizeof(char*));
		waveTerrainDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for x */
		port_descriptors[WAVETERRAIN_XB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[WAVETERRAIN_XB] =
		 D_("x");
		port_range_hints[WAVETERRAIN_XB].HintDescriptor = 0;

		/* Parameters for y */
		port_descriptors[WAVETERRAIN_YB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[WAVETERRAIN_YB] =
		 D_("y");
		port_range_hints[WAVETERRAIN_YB].HintDescriptor = 0;

		/* Parameters for z */
		port_descriptors[WAVETERRAIN_ZB] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[WAVETERRAIN_ZB] =
		 D_("z");
		port_range_hints[WAVETERRAIN_ZB].HintDescriptor = 0;

		waveTerrainDescriptor->activate = NULL;
		waveTerrainDescriptor->cleanup = cleanupWaveTerrain;
		waveTerrainDescriptor->connect_port = connectPortWaveTerrain;
		waveTerrainDescriptor->deactivate = NULL;
		waveTerrainDescriptor->instantiate = instantiateWaveTerrain;
		waveTerrainDescriptor->run = runWaveTerrain;
		waveTerrainDescriptor->run_adding = runAddingWaveTerrain;
		waveTerrainDescriptor->set_run_adding_gain = setRunAddingGainWaveTerrain;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (waveTerrainDescriptor) {
		free((LADSPA_PortDescriptor *)waveTerrainDescriptor->PortDescriptors);
		free((char **)waveTerrainDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)waveTerrainDescriptor->PortRangeHints);
		free(waveTerrainDescriptor);
	}

}
