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

#line 10 "sin_cos_1881.xml"

#include "ladspa-util.h"

#define SINCOS_FREQ                    0
#define SINCOS_PITCH                   1
#define SINCOS_SINE                    2
#define SINCOS_COSINE                  3

static LADSPA_Descriptor *sinCosDescriptor = NULL;

typedef struct {
	LADSPA_Data *freq;
	LADSPA_Data *pitch;
	LADSPA_Data *sine;
	LADSPA_Data *cosine;
	float        fs;
	double       last_om;
	double       phi;
	LADSPA_Data run_adding_gain;
} SinCos;

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
		return sinCosDescriptor;
	default:
		return NULL;
	}
}

static void cleanupSinCos(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortSinCos(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	SinCos *plugin;

	plugin = (SinCos *)instance;
	switch (port) {
	case SINCOS_FREQ:
		plugin->freq = data;
		break;
	case SINCOS_PITCH:
		plugin->pitch = data;
		break;
	case SINCOS_SINE:
		plugin->sine = data;
		break;
	case SINCOS_COSINE:
		plugin->cosine = data;
		break;
	}
}

static LADSPA_Handle instantiateSinCos(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	SinCos *plugin_data = (SinCos *)malloc(sizeof(SinCos));
	float fs;
	double last_om;
	double phi;

#line 21 "sin_cos_1881.xml"
	fs = (float)s_rate;
	phi = 0.0;
	last_om = 0.0;

	plugin_data->fs = fs;
	plugin_data->last_om = last_om;
	plugin_data->phi = phi;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runSinCos(LADSPA_Handle instance, unsigned long sample_count) {
	SinCos *plugin_data = (SinCos *)instance;

	/* Base frequency (Hz) (float value) */
	const LADSPA_Data freq = *(plugin_data->freq);

	/* Pitch offset (float value) */
	const LADSPA_Data pitch = *(plugin_data->pitch);

	/* Sine output (array of floats of length sample_count) */
	LADSPA_Data * const sine = plugin_data->sine;

	/* Cosine output (array of floats of length sample_count) */
	LADSPA_Data * const cosine = plugin_data->cosine;
	float fs = plugin_data->fs;
	double last_om = plugin_data->last_om;
	double phi = plugin_data->phi;

#line 27 "sin_cos_1881.xml"
	unsigned long pos;
	const double target_om = 2.0 * M_PI * f_clamp(freq, 0.0f, 0.5f) * pow(2.0, f_clamp(pitch, 0.0f, 16.0f)) / fs;
	const double om_d = (target_om - last_om) / (double)sample_count;
	double om = last_om;

	for (pos = 0; pos < sample_count; pos++) {
	  buffer_write(sine[pos], sin(phi));
	  buffer_write(cosine[pos], cos(phi));
	  om += om_d;
	  phi += om;
	}
	while (phi > 2.0 * M_PI) {
	  phi -= 2.0 * M_PI;
	}

	plugin_data->phi = phi;
	plugin_data->last_om = target_om;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainSinCos(LADSPA_Handle instance, LADSPA_Data gain) {
	((SinCos *)instance)->run_adding_gain = gain;
}

static void runAddingSinCos(LADSPA_Handle instance, unsigned long sample_count) {
	SinCos *plugin_data = (SinCos *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Base frequency (Hz) (float value) */
	const LADSPA_Data freq = *(plugin_data->freq);

	/* Pitch offset (float value) */
	const LADSPA_Data pitch = *(plugin_data->pitch);

	/* Sine output (array of floats of length sample_count) */
	LADSPA_Data * const sine = plugin_data->sine;

	/* Cosine output (array of floats of length sample_count) */
	LADSPA_Data * const cosine = plugin_data->cosine;
	float fs = plugin_data->fs;
	double last_om = plugin_data->last_om;
	double phi = plugin_data->phi;

#line 27 "sin_cos_1881.xml"
	unsigned long pos;
	const double target_om = 2.0 * M_PI * f_clamp(freq, 0.0f, 0.5f) * pow(2.0, f_clamp(pitch, 0.0f, 16.0f)) / fs;
	const double om_d = (target_om - last_om) / (double)sample_count;
	double om = last_om;

	for (pos = 0; pos < sample_count; pos++) {
	  buffer_write(sine[pos], sin(phi));
	  buffer_write(cosine[pos], cos(phi));
	  om += om_d;
	  phi += om;
	}
	while (phi > 2.0 * M_PI) {
	  phi -= 2.0 * M_PI;
	}

	plugin_data->phi = phi;
	plugin_data->last_om = target_om;
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


	sinCosDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (sinCosDescriptor) {
		sinCosDescriptor->UniqueID = 1881;
		sinCosDescriptor->Label = "sinCos";
		sinCosDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		sinCosDescriptor->Name =
		 D_("Sine + cosine oscillator");
		sinCosDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		sinCosDescriptor->Copyright =
		 "GPL";
		sinCosDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		sinCosDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		sinCosDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		sinCosDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Base frequency (Hz) */
		port_descriptors[SINCOS_FREQ] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SINCOS_FREQ] =
		 D_("Base frequency (Hz)");
		port_range_hints[SINCOS_FREQ].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_440 | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_LOGARITHMIC;
		port_range_hints[SINCOS_FREQ].LowerBound = 0.000001;
		port_range_hints[SINCOS_FREQ].UpperBound = 0.5;

		/* Parameters for Pitch offset */
		port_descriptors[SINCOS_PITCH] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SINCOS_PITCH] =
		 D_("Pitch offset");
		port_range_hints[SINCOS_PITCH].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[SINCOS_PITCH].LowerBound = 0;
		port_range_hints[SINCOS_PITCH].UpperBound = 8;

		/* Parameters for Sine output */
		port_descriptors[SINCOS_SINE] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[SINCOS_SINE] =
		 D_("Sine output");
		port_range_hints[SINCOS_SINE].HintDescriptor = 0;

		/* Parameters for Cosine output */
		port_descriptors[SINCOS_COSINE] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[SINCOS_COSINE] =
		 D_("Cosine output");
		port_range_hints[SINCOS_COSINE].HintDescriptor = 0;

		sinCosDescriptor->activate = NULL;
		sinCosDescriptor->cleanup = cleanupSinCos;
		sinCosDescriptor->connect_port = connectPortSinCos;
		sinCosDescriptor->deactivate = NULL;
		sinCosDescriptor->instantiate = instantiateSinCos;
		sinCosDescriptor->run = runSinCos;
		sinCosDescriptor->run_adding = runAddingSinCos;
		sinCosDescriptor->set_run_adding_gain = setRunAddingGainSinCos;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (sinCosDescriptor) {
		free((LADSPA_PortDescriptor *)sinCosDescriptor->PortDescriptors);
		free((char **)sinCosDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)sinCosDescriptor->PortRangeHints);
		free(sinCosDescriptor);
	}

}
