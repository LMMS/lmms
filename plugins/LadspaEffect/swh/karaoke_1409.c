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


#define KARAOKE_GAIN                   0
#define KARAOKE_LIN                    1
#define KARAOKE_RIN                    2
#define KARAOKE_LOUT                   3
#define KARAOKE_ROUT                   4

static LADSPA_Descriptor *karaokeDescriptor = NULL;

typedef struct {
	LADSPA_Data *gain;
	LADSPA_Data *lin;
	LADSPA_Data *rin;
	LADSPA_Data *lout;
	LADSPA_Data *rout;
	LADSPA_Data run_adding_gain;
} Karaoke;

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
		return karaokeDescriptor;
	default:
		return NULL;
	}
}

static void cleanupKaraoke(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortKaraoke(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Karaoke *plugin;

	plugin = (Karaoke *)instance;
	switch (port) {
	case KARAOKE_GAIN:
		plugin->gain = data;
		break;
	case KARAOKE_LIN:
		plugin->lin = data;
		break;
	case KARAOKE_RIN:
		plugin->rin = data;
		break;
	case KARAOKE_LOUT:
		plugin->lout = data;
		break;
	case KARAOKE_ROUT:
		plugin->rout = data;
		break;
	}
}

static LADSPA_Handle instantiateKaraoke(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Karaoke *plugin_data = (Karaoke *)malloc(sizeof(Karaoke));
	plugin_data->run_adding_gain = 1.0f;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runKaraoke(LADSPA_Handle instance, unsigned long sample_count) {
	Karaoke *plugin_data = (Karaoke *)instance;

	/* Vocal volume (dB) (float value) */
	const LADSPA_Data gain = *(plugin_data->gain);

	/* Left in (array of floats of length sample_count) */
	const LADSPA_Data * const lin = plugin_data->lin;

	/* Right in (array of floats of length sample_count) */
	const LADSPA_Data * const rin = plugin_data->rin;

	/* Left out (array of floats of length sample_count) */
	LADSPA_Data * const lout = plugin_data->lout;

	/* Right out (array of floats of length sample_count) */
	LADSPA_Data * const rout = plugin_data->rout;

#line 17 "karaoke_1409.xml"
	unsigned long pos;
	float coef = pow(10.0f, gain * 0.05f) * 0.5f;
	float m, s;

	for (pos = 0; pos < sample_count; pos++) {
	  m = lin[pos] + rin[pos];
	  s = lin[pos] - rin[pos];
	  buffer_write(lout[pos], m * coef + s * 0.5f);
	  buffer_write(rout[pos], m * coef - s * 0.5f);
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainKaraoke(LADSPA_Handle instance, LADSPA_Data gain) {
	((Karaoke *)instance)->run_adding_gain = gain;
}

static void runAddingKaraoke(LADSPA_Handle instance, unsigned long sample_count) {
	Karaoke *plugin_data = (Karaoke *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Vocal volume (dB) (float value) */
	const LADSPA_Data gain = *(plugin_data->gain);

	/* Left in (array of floats of length sample_count) */
	const LADSPA_Data * const lin = plugin_data->lin;

	/* Right in (array of floats of length sample_count) */
	const LADSPA_Data * const rin = plugin_data->rin;

	/* Left out (array of floats of length sample_count) */
	LADSPA_Data * const lout = plugin_data->lout;

	/* Right out (array of floats of length sample_count) */
	LADSPA_Data * const rout = plugin_data->rout;

#line 17 "karaoke_1409.xml"
	unsigned long pos;
	float coef = pow(10.0f, gain * 0.05f) * 0.5f;
	float m, s;

	for (pos = 0; pos < sample_count; pos++) {
	  m = lin[pos] + rin[pos];
	  s = lin[pos] - rin[pos];
	  buffer_write(lout[pos], m * coef + s * 0.5f);
	  buffer_write(rout[pos], m * coef - s * 0.5f);
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


	karaokeDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (karaokeDescriptor) {
		karaokeDescriptor->UniqueID = 1409;
		karaokeDescriptor->Label = "karaoke";
		karaokeDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		karaokeDescriptor->Name =
		 D_("Karaoke");
		karaokeDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		karaokeDescriptor->Copyright =
		 "GPL";
		karaokeDescriptor->PortCount = 5;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(5,
		 sizeof(LADSPA_PortDescriptor));
		karaokeDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(5,
		 sizeof(LADSPA_PortRangeHint));
		karaokeDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(5, sizeof(char*));
		karaokeDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Vocal volume (dB) */
		port_descriptors[KARAOKE_GAIN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[KARAOKE_GAIN] =
		 D_("Vocal volume (dB)");
		port_range_hints[KARAOKE_GAIN].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[KARAOKE_GAIN].LowerBound = -70;
		port_range_hints[KARAOKE_GAIN].UpperBound = 0;

		/* Parameters for Left in */
		port_descriptors[KARAOKE_LIN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[KARAOKE_LIN] =
		 D_("Left in");
		port_range_hints[KARAOKE_LIN].HintDescriptor = 0;

		/* Parameters for Right in */
		port_descriptors[KARAOKE_RIN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[KARAOKE_RIN] =
		 D_("Right in");
		port_range_hints[KARAOKE_RIN].HintDescriptor = 0;

		/* Parameters for Left out */
		port_descriptors[KARAOKE_LOUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[KARAOKE_LOUT] =
		 D_("Left out");
		port_range_hints[KARAOKE_LOUT].HintDescriptor = 0;

		/* Parameters for Right out */
		port_descriptors[KARAOKE_ROUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[KARAOKE_ROUT] =
		 D_("Right out");
		port_range_hints[KARAOKE_ROUT].HintDescriptor = 0;

		karaokeDescriptor->activate = NULL;
		karaokeDescriptor->cleanup = cleanupKaraoke;
		karaokeDescriptor->connect_port = connectPortKaraoke;
		karaokeDescriptor->deactivate = NULL;
		karaokeDescriptor->instantiate = instantiateKaraoke;
		karaokeDescriptor->run = runKaraoke;
		karaokeDescriptor->run_adding = runAddingKaraoke;
		karaokeDescriptor->set_run_adding_gain = setRunAddingGainKaraoke;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (karaokeDescriptor) {
		free((LADSPA_PortDescriptor *)karaokeDescriptor->PortDescriptors);
		free((char **)karaokeDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)karaokeDescriptor->PortRangeHints);
		free(karaokeDescriptor);
	}

}
