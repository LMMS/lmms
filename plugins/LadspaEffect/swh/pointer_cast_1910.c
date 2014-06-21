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

#line 10 "pointer_cast_1910.xml"

#include <limits.h>

#include "ladspa-util.h"
#include "util/biquad.h"

typedef union {
  LADSPA_Data fp;
  int         in;
} pcast;

#define POINTERCASTDISTORTION_CUTOFF   0
#define POINTERCASTDISTORTION_WET      1
#define POINTERCASTDISTORTION_INPUT    2
#define POINTERCASTDISTORTION_OUTPUT   3

static LADSPA_Descriptor *pointerCastDistortionDescriptor = NULL;

typedef struct {
	LADSPA_Data *cutoff;
	LADSPA_Data *wet;
	LADSPA_Data *input;
	LADSPA_Data *output;
	biquad *     filt;
	float        fs;
	LADSPA_Data run_adding_gain;
} PointerCastDistortion;

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
		return pointerCastDistortionDescriptor;
	default:
		return NULL;
	}
}

static void activatePointerCastDistortion(LADSPA_Handle instance) {
	PointerCastDistortion *plugin_data = (PointerCastDistortion *)instance;
	biquad *filt = plugin_data->filt;
	float fs = plugin_data->fs;
#line 36 "pointer_cast_1910.xml"
	biquad_init(filt);
	plugin_data->filt = filt;
	plugin_data->fs = fs;

}

static void cleanupPointerCastDistortion(LADSPA_Handle instance) {
#line 59 "pointer_cast_1910.xml"
	PointerCastDistortion *plugin_data = (PointerCastDistortion *)instance;
	free(plugin_data->filt);
	free(instance);
}

static void connectPortPointerCastDistortion(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	PointerCastDistortion *plugin;

	plugin = (PointerCastDistortion *)instance;
	switch (port) {
	case POINTERCASTDISTORTION_CUTOFF:
		plugin->cutoff = data;
		break;
	case POINTERCASTDISTORTION_WET:
		plugin->wet = data;
		break;
	case POINTERCASTDISTORTION_INPUT:
		plugin->input = data;
		break;
	case POINTERCASTDISTORTION_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiatePointerCastDistortion(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	PointerCastDistortion *plugin_data = (PointerCastDistortion *)malloc(sizeof(PointerCastDistortion));
	biquad *filt = NULL;
	float fs;

#line 31 "pointer_cast_1910.xml"
	filt = malloc(sizeof(biquad));
	fs = s_rate;

	plugin_data->filt = filt;
	plugin_data->fs = fs;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runPointerCastDistortion(LADSPA_Handle instance, unsigned long sample_count) {
	PointerCastDistortion *plugin_data = (PointerCastDistortion *)instance;

	/* Effect cutoff freq (Hz) (float value) */
	const LADSPA_Data cutoff = *(plugin_data->cutoff);

	/* Dry/wet mix (float value) */
	const LADSPA_Data wet = *(plugin_data->wet);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	biquad * filt = plugin_data->filt;
	float fs = plugin_data->fs;

#line 40 "pointer_cast_1910.xml"
	unsigned long pos;
	const float filt_scale = cutoff < 50.0f ? cutoff / 50.0f : 1.0f;

	lp_set_params(filt, cutoff, 1.0f, fs);

	for (pos = 0; pos < sample_count; pos++) {
	  pcast val;
	  float sign, filt_val, dist_val;

	  filt_val = biquad_run(filt, input[pos]) * filt_scale;
	  sign = filt_val < 0.0f ? -1.0f : 1.0f;
	  val.fp = fabs(filt_val);
	  dist_val = sign * (LADSPA_Data)val.in / (LADSPA_Data)INT_MAX +
	             (input[pos] - filt_val);
	  buffer_write(output[pos], LIN_INTERP(wet, input[pos], dist_val));
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainPointerCastDistortion(LADSPA_Handle instance, LADSPA_Data gain) {
	((PointerCastDistortion *)instance)->run_adding_gain = gain;
}

static void runAddingPointerCastDistortion(LADSPA_Handle instance, unsigned long sample_count) {
	PointerCastDistortion *plugin_data = (PointerCastDistortion *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Effect cutoff freq (Hz) (float value) */
	const LADSPA_Data cutoff = *(plugin_data->cutoff);

	/* Dry/wet mix (float value) */
	const LADSPA_Data wet = *(plugin_data->wet);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	biquad * filt = plugin_data->filt;
	float fs = plugin_data->fs;

#line 40 "pointer_cast_1910.xml"
	unsigned long pos;
	const float filt_scale = cutoff < 50.0f ? cutoff / 50.0f : 1.0f;

	lp_set_params(filt, cutoff, 1.0f, fs);

	for (pos = 0; pos < sample_count; pos++) {
	  pcast val;
	  float sign, filt_val, dist_val;

	  filt_val = biquad_run(filt, input[pos]) * filt_scale;
	  sign = filt_val < 0.0f ? -1.0f : 1.0f;
	  val.fp = fabs(filt_val);
	  dist_val = sign * (LADSPA_Data)val.in / (LADSPA_Data)INT_MAX +
	             (input[pos] - filt_val);
	  buffer_write(output[pos], LIN_INTERP(wet, input[pos], dist_val));
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


	pointerCastDistortionDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (pointerCastDistortionDescriptor) {
		pointerCastDistortionDescriptor->UniqueID = 1910;
		pointerCastDistortionDescriptor->Label = "pointerCastDistortion";
		pointerCastDistortionDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		pointerCastDistortionDescriptor->Name =
		 D_("Pointer cast distortion");
		pointerCastDistortionDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		pointerCastDistortionDescriptor->Copyright =
		 "GPL";
		pointerCastDistortionDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		pointerCastDistortionDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		pointerCastDistortionDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		pointerCastDistortionDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Effect cutoff freq (Hz) */
		port_descriptors[POINTERCASTDISTORTION_CUTOFF] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[POINTERCASTDISTORTION_CUTOFF] =
		 D_("Effect cutoff freq (Hz)");
		port_range_hints[POINTERCASTDISTORTION_CUTOFF].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_LOGARITHMIC | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[POINTERCASTDISTORTION_CUTOFF].LowerBound = 0.0001;
		port_range_hints[POINTERCASTDISTORTION_CUTOFF].UpperBound = 0.3;

		/* Parameters for Dry/wet mix */
		port_descriptors[POINTERCASTDISTORTION_WET] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[POINTERCASTDISTORTION_WET] =
		 D_("Dry/wet mix");
		port_range_hints[POINTERCASTDISTORTION_WET].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[POINTERCASTDISTORTION_WET].LowerBound = 0;
		port_range_hints[POINTERCASTDISTORTION_WET].UpperBound = 1;

		/* Parameters for Input */
		port_descriptors[POINTERCASTDISTORTION_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[POINTERCASTDISTORTION_INPUT] =
		 D_("Input");
		port_range_hints[POINTERCASTDISTORTION_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[POINTERCASTDISTORTION_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[POINTERCASTDISTORTION_OUTPUT] =
		 D_("Output");
		port_range_hints[POINTERCASTDISTORTION_OUTPUT].HintDescriptor = 0;

		pointerCastDistortionDescriptor->activate = activatePointerCastDistortion;
		pointerCastDistortionDescriptor->cleanup = cleanupPointerCastDistortion;
		pointerCastDistortionDescriptor->connect_port = connectPortPointerCastDistortion;
		pointerCastDistortionDescriptor->deactivate = NULL;
		pointerCastDistortionDescriptor->instantiate = instantiatePointerCastDistortion;
		pointerCastDistortionDescriptor->run = runPointerCastDistortion;
		pointerCastDistortionDescriptor->run_adding = runAddingPointerCastDistortion;
		pointerCastDistortionDescriptor->set_run_adding_gain = setRunAddingGainPointerCastDistortion;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (pointerCastDistortionDescriptor) {
		free((LADSPA_PortDescriptor *)pointerCastDistortionDescriptor->PortDescriptors);
		free((char **)pointerCastDistortionDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)pointerCastDistortionDescriptor->PortRangeHints);
		free(pointerCastDistortionDescriptor);
	}

}
