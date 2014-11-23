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

#line 9 "gsm_1215.xml"

#include <stdlib.h>
#include "ladspa-util.h"
#include "gsm/gsm.h"
#include "util/biquad.h"

#define SCALE 32768.0f
#define SCALE_R 0.0000305175f

int bits[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

#define GSM_DRYWET                     0
#define GSM_PASSES                     1
#define GSM_ERROR                      2
#define GSM_INPUT                      3
#define GSM_OUTPUT                     4
#define GSM_LATENCY                    5

static LADSPA_Descriptor *gsmDescriptor = NULL;

typedef struct {
	LADSPA_Data *drywet;
	LADSPA_Data *passes;
	LADSPA_Data *error;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data *latency;
	biquad *     blf;
	int          count;
	LADSPA_Data *dry;
	gsm_signal * dst;
	float        fs;
	gsm          handle;
	int          resamp;
	float        rsf;
	gsm_signal * src;
	LADSPA_Data run_adding_gain;
} Gsm;

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
		return gsmDescriptor;
	default:
		return NULL;
	}
}

static void activateGsm(LADSPA_Handle instance) {
	Gsm *plugin_data = (Gsm *)instance;
	biquad *blf = plugin_data->blf;
	int count = plugin_data->count;
	LADSPA_Data *dry = plugin_data->dry;
	gsm_signal *dst = plugin_data->dst;
	float fs = plugin_data->fs;
	gsm handle = plugin_data->handle;
	int resamp = plugin_data->resamp;
	float rsf = plugin_data->rsf;
	gsm_signal *src = plugin_data->src;
#line 41 "gsm_1215.xml"
	count = 0;
	memset(src, 0, sizeof(gsm_signal) * 160);
	memset(dst, 0, sizeof(gsm_signal) * 163);
	memset(dry, 0, sizeof(LADSPA_Data) * 160 * resamp);
	handle = gsm_create();
	biquad_init(blf);
	hs_set_params(blf, 3500.0f, -50.0f, 0.7f, fs);
	plugin_data->blf = blf;
	plugin_data->count = count;
	plugin_data->dry = dry;
	plugin_data->dst = dst;
	plugin_data->fs = fs;
	plugin_data->handle = handle;
	plugin_data->resamp = resamp;
	plugin_data->rsf = rsf;
	plugin_data->src = src;

}

static void cleanupGsm(LADSPA_Handle instance) {
#line 51 "gsm_1215.xml"
	Gsm *plugin_data = (Gsm *)instance;
	free(plugin_data->src);
	free(plugin_data->dst);
	free(plugin_data->dry);
	free(plugin_data->blf);
	if (plugin_data->handle) {
	  gsm_destroy(plugin_data->handle);
	}
	free(instance);
}

static void connectPortGsm(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Gsm *plugin;

	plugin = (Gsm *)instance;
	switch (port) {
	case GSM_DRYWET:
		plugin->drywet = data;
		break;
	case GSM_PASSES:
		plugin->passes = data;
		break;
	case GSM_ERROR:
		plugin->error = data;
		break;
	case GSM_INPUT:
		plugin->input = data;
		break;
	case GSM_OUTPUT:
		plugin->output = data;
		break;
	case GSM_LATENCY:
		plugin->latency = data;
		break;
	}
}

static LADSPA_Handle instantiateGsm(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Gsm *plugin_data = (Gsm *)malloc(sizeof(Gsm));
	biquad *blf = NULL;
	int count;
	LADSPA_Data *dry = NULL;
	gsm_signal *dst = NULL;
	float fs;
	gsm handle;
	int resamp;
	float rsf;
	gsm_signal *src = NULL;

#line 27 "gsm_1215.xml"
	count = 0;
	resamp = s_rate / 8000;
	fs = s_rate;
	rsf = SCALE / (float)resamp;
	src = malloc(sizeof(gsm_signal) * 160);
	dst = malloc(sizeof(gsm_signal) * 163);
	dry = malloc(sizeof(LADSPA_Data) * 160 * resamp);
	handle = NULL;

	blf = malloc(sizeof(biquad));
	biquad_init(blf);

	plugin_data->blf = blf;
	plugin_data->count = count;
	plugin_data->dry = dry;
	plugin_data->dst = dst;
	plugin_data->fs = fs;
	plugin_data->handle = handle;
	plugin_data->resamp = resamp;
	plugin_data->rsf = rsf;
	plugin_data->src = src;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runGsm(LADSPA_Handle instance, unsigned long sample_count) {
	Gsm *plugin_data = (Gsm *)instance;

	/* Dry/wet mix (float value) */
	const LADSPA_Data drywet = *(plugin_data->drywet);

	/* Number of passes (float value) */
	const LADSPA_Data passes = *(plugin_data->passes);

	/* Error rate (bits/block) (float value) */
	const LADSPA_Data error = *(plugin_data->error);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	biquad * blf = plugin_data->blf;
	int count = plugin_data->count;
	LADSPA_Data * dry = plugin_data->dry;
	gsm_signal * dst = plugin_data->dst;
	gsm handle = plugin_data->handle;
	int resamp = plugin_data->resamp;
	float rsf = plugin_data->rsf;
	gsm_signal * src = plugin_data->src;

#line 61 "gsm_1215.xml"
	unsigned long pos;
	gsm_frame frame;
	int samp;
	float part;
	int error_rate = f_round(error);
	int num_passes = f_round(passes);

	for (pos = 0; pos < sample_count; pos++) {

	  // oversample into buffer down to aprox 8kHz, 13bit
	  src[count / resamp] += f_round(biquad_run(blf, input[pos]) * rsf);

	  // interpolate output, so it doesn't sound totaly awful
	  samp = count / resamp;
	  part = (float)count / (float)resamp - (float)samp;
	  buffer_write(output[pos], cube_interp(part, dst[samp], dst[samp+1], dst[samp+2], dst[samp+3]) * SCALE_R * drywet + dry[count] * (1.0f - drywet));

	  // Maintain delayed, dry buffer.
	  dry[count] = input[pos];

	  count++;

	  // If we have a full, downsampled buffer then run the encode +
	  // decode process.
	  if (count >= 160 * resamp) {
	          int i, j;
	          gsm_signal *in;

	          count = 0;
	          dst[0] = dst[160];
	          dst[1] = dst[161];
	          dst[2] = dst[162];

	          in = src;
	          for (j=0; j<num_passes; j++) {
	                  gsm_encode(handle, in, frame);
	                  for (i=0; i < error_rate; i++) {
	                          frame[1 + (rand() % 32)] ^= bits[rand() % 8];
	                  }
	                  gsm_decode(handle, frame, dst+3);
	                  in = dst+3;
	          }
	          if (num_passes == 0) {
	                  for (j=0; j < 160; j++) {
	                          dst[j + 3] = src[j];
	                  }
	          }
	          memset(src, 0, sizeof(gsm_signal) * 160);
	  }
	}

	plugin_data->count = count;

	*(plugin_data->latency) = 160 * resamp;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainGsm(LADSPA_Handle instance, LADSPA_Data gain) {
	((Gsm *)instance)->run_adding_gain = gain;
}

static void runAddingGsm(LADSPA_Handle instance, unsigned long sample_count) {
	Gsm *plugin_data = (Gsm *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Dry/wet mix (float value) */
	const LADSPA_Data drywet = *(plugin_data->drywet);

	/* Number of passes (float value) */
	const LADSPA_Data passes = *(plugin_data->passes);

	/* Error rate (bits/block) (float value) */
	const LADSPA_Data error = *(plugin_data->error);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	biquad * blf = plugin_data->blf;
	int count = plugin_data->count;
	LADSPA_Data * dry = plugin_data->dry;
	gsm_signal * dst = plugin_data->dst;
	gsm handle = plugin_data->handle;
	int resamp = plugin_data->resamp;
	float rsf = plugin_data->rsf;
	gsm_signal * src = plugin_data->src;

#line 61 "gsm_1215.xml"
	unsigned long pos;
	gsm_frame frame;
	int samp;
	float part;
	int error_rate = f_round(error);
	int num_passes = f_round(passes);

	for (pos = 0; pos < sample_count; pos++) {

	  // oversample into buffer down to aprox 8kHz, 13bit
	  src[count / resamp] += f_round(biquad_run(blf, input[pos]) * rsf);

	  // interpolate output, so it doesn't sound totaly awful
	  samp = count / resamp;
	  part = (float)count / (float)resamp - (float)samp;
	  buffer_write(output[pos], cube_interp(part, dst[samp], dst[samp+1], dst[samp+2], dst[samp+3]) * SCALE_R * drywet + dry[count] * (1.0f - drywet));

	  // Maintain delayed, dry buffer.
	  dry[count] = input[pos];

	  count++;

	  // If we have a full, downsampled buffer then run the encode +
	  // decode process.
	  if (count >= 160 * resamp) {
	          int i, j;
	          gsm_signal *in;

	          count = 0;
	          dst[0] = dst[160];
	          dst[1] = dst[161];
	          dst[2] = dst[162];

	          in = src;
	          for (j=0; j<num_passes; j++) {
	                  gsm_encode(handle, in, frame);
	                  for (i=0; i < error_rate; i++) {
	                          frame[1 + (rand() % 32)] ^= bits[rand() % 8];
	                  }
	                  gsm_decode(handle, frame, dst+3);
	                  in = dst+3;
	          }
	          if (num_passes == 0) {
	                  for (j=0; j < 160; j++) {
	                          dst[j + 3] = src[j];
	                  }
	          }
	          memset(src, 0, sizeof(gsm_signal) * 160);
	  }
	}

	plugin_data->count = count;

	*(plugin_data->latency) = 160 * resamp;
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


	gsmDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (gsmDescriptor) {
		gsmDescriptor->UniqueID = 1215;
		gsmDescriptor->Label = "gsm";
		gsmDescriptor->Properties =
		 0;
		gsmDescriptor->Name =
		 D_("GSM simulator");
		gsmDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		gsmDescriptor->Copyright =
		 "GPL";
		gsmDescriptor->PortCount = 6;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(6,
		 sizeof(LADSPA_PortDescriptor));
		gsmDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(6,
		 sizeof(LADSPA_PortRangeHint));
		gsmDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(6, sizeof(char*));
		gsmDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Dry/wet mix */
		port_descriptors[GSM_DRYWET] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GSM_DRYWET] =
		 D_("Dry/wet mix");
		port_range_hints[GSM_DRYWET].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[GSM_DRYWET].LowerBound = 0;
		port_range_hints[GSM_DRYWET].UpperBound = 1;

		/* Parameters for Number of passes */
		port_descriptors[GSM_PASSES] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GSM_PASSES] =
		 D_("Number of passes");
		port_range_hints[GSM_PASSES].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_1;
		port_range_hints[GSM_PASSES].LowerBound = 0;
		port_range_hints[GSM_PASSES].UpperBound = 10;

		/* Parameters for Error rate (bits/block) */
		port_descriptors[GSM_ERROR] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GSM_ERROR] =
		 D_("Error rate (bits/block)");
		port_range_hints[GSM_ERROR].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[GSM_ERROR].LowerBound = 0;
		port_range_hints[GSM_ERROR].UpperBound = 30;

		/* Parameters for Input */
		port_descriptors[GSM_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[GSM_INPUT] =
		 D_("Input");
		port_range_hints[GSM_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[GSM_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[GSM_OUTPUT] =
		 D_("Output");
		port_range_hints[GSM_OUTPUT].HintDescriptor = 0;

		/* Parameters for latency */
		port_descriptors[GSM_LATENCY] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
		port_names[GSM_LATENCY] =
		 D_("latency");
		port_range_hints[GSM_LATENCY].HintDescriptor = 0;

		gsmDescriptor->activate = activateGsm;
		gsmDescriptor->cleanup = cleanupGsm;
		gsmDescriptor->connect_port = connectPortGsm;
		gsmDescriptor->deactivate = NULL;
		gsmDescriptor->instantiate = instantiateGsm;
		gsmDescriptor->run = runGsm;
		gsmDescriptor->run_adding = runAddingGsm;
		gsmDescriptor->set_run_adding_gain = setRunAddingGainGsm;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (gsmDescriptor) {
		free((LADSPA_PortDescriptor *)gsmDescriptor->PortDescriptors);
		free((char **)gsmDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)gsmDescriptor->PortRangeHints);
		free(gsmDescriptor);
	}

}
