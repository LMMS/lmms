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

#line 10 "lcr_delay_1436.xml"

#include "ladspa-util.h"
#include "util/biquad.h"

#define LCRDELAY_LDEL                  0
#define LCRDELAY_LLEV                  1
#define LCRDELAY_CDEL                  2
#define LCRDELAY_CLEV                  3
#define LCRDELAY_RDEL                  4
#define LCRDELAY_RLEV                  5
#define LCRDELAY_FEEDBACK              6
#define LCRDELAY_HIGH_D                7
#define LCRDELAY_LOW_D                 8
#define LCRDELAY_SPREAD                9
#define LCRDELAY_WET                   10
#define LCRDELAY_IN_L                  11
#define LCRDELAY_IN_R                  12
#define LCRDELAY_OUT_L                 13
#define LCRDELAY_OUT_R                 14

static LADSPA_Descriptor *lcrDelayDescriptor = NULL;

typedef struct {
	LADSPA_Data *ldel;
	LADSPA_Data *llev;
	LADSPA_Data *cdel;
	LADSPA_Data *clev;
	LADSPA_Data *rdel;
	LADSPA_Data *rlev;
	LADSPA_Data *feedback;
	LADSPA_Data *high_d;
	LADSPA_Data *low_d;
	LADSPA_Data *spread;
	LADSPA_Data *wet;
	LADSPA_Data *in_l;
	LADSPA_Data *in_r;
	LADSPA_Data *out_l;
	LADSPA_Data *out_r;
	LADSPA_Data *buffer;
	unsigned int buffer_mask;
	unsigned int buffer_pos;
	biquad *     filters;
	float        fs;
	float        last_cd;
	float        last_cl;
	float        last_ld;
	float        last_ll;
	float        last_rd;
	float        last_rl;
	LADSPA_Data run_adding_gain;
} LcrDelay;

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
		return lcrDelayDescriptor;
	default:
		return NULL;
	}
}

static void activateLcrDelay(LADSPA_Handle instance) {
	LcrDelay *plugin_data = (LcrDelay *)instance;
	LADSPA_Data *buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	unsigned int buffer_pos = plugin_data->buffer_pos;
	biquad *filters = plugin_data->filters;
	float fs = plugin_data->fs;
	float last_cd = plugin_data->last_cd;
	float last_cl = plugin_data->last_cl;
	float last_ld = plugin_data->last_ld;
	float last_ll = plugin_data->last_ll;
	float last_rd = plugin_data->last_rd;
	float last_rl = plugin_data->last_rl;
#line 41 "lcr_delay_1436.xml"
	memset(buffer, 0, (buffer_mask + 1) * sizeof(LADSPA_Data));
	last_ll = 0.0f;
	last_cl = 0.0f;
	last_rl = 0.0f;
	last_ld = 0.0f;
	last_cd = 0.0f;
	last_rd = 0.0f;
	biquad_init(filters);
	biquad_init(filters + 1);
	plugin_data->buffer = buffer;
	plugin_data->buffer_mask = buffer_mask;
	plugin_data->buffer_pos = buffer_pos;
	plugin_data->filters = filters;
	plugin_data->fs = fs;
	plugin_data->last_cd = last_cd;
	plugin_data->last_cl = last_cl;
	plugin_data->last_ld = last_ld;
	plugin_data->last_ll = last_ll;
	plugin_data->last_rd = last_rd;
	plugin_data->last_rl = last_rl;

}

static void cleanupLcrDelay(LADSPA_Handle instance) {
#line 53 "lcr_delay_1436.xml"
	LcrDelay *plugin_data = (LcrDelay *)instance;
	free(plugin_data->filters);
	free(plugin_data->buffer);
	free(instance);
}

static void connectPortLcrDelay(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	LcrDelay *plugin;

	plugin = (LcrDelay *)instance;
	switch (port) {
	case LCRDELAY_LDEL:
		plugin->ldel = data;
		break;
	case LCRDELAY_LLEV:
		plugin->llev = data;
		break;
	case LCRDELAY_CDEL:
		plugin->cdel = data;
		break;
	case LCRDELAY_CLEV:
		plugin->clev = data;
		break;
	case LCRDELAY_RDEL:
		plugin->rdel = data;
		break;
	case LCRDELAY_RLEV:
		plugin->rlev = data;
		break;
	case LCRDELAY_FEEDBACK:
		plugin->feedback = data;
		break;
	case LCRDELAY_HIGH_D:
		plugin->high_d = data;
		break;
	case LCRDELAY_LOW_D:
		plugin->low_d = data;
		break;
	case LCRDELAY_SPREAD:
		plugin->spread = data;
		break;
	case LCRDELAY_WET:
		plugin->wet = data;
		break;
	case LCRDELAY_IN_L:
		plugin->in_l = data;
		break;
	case LCRDELAY_IN_R:
		plugin->in_r = data;
		break;
	case LCRDELAY_OUT_L:
		plugin->out_l = data;
		break;
	case LCRDELAY_OUT_R:
		plugin->out_r = data;
		break;
	}
}

static LADSPA_Handle instantiateLcrDelay(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	LcrDelay *plugin_data = (LcrDelay *)malloc(sizeof(LcrDelay));
	LADSPA_Data *buffer = NULL;
	unsigned int buffer_mask;
	unsigned int buffer_pos;
	biquad *filters = NULL;
	float fs;
	float last_cd;
	float last_cl;
	float last_ld;
	float last_ll;
	float last_rd;
	float last_rl;

#line 21 "lcr_delay_1436.xml"
	int buffer_size = 32768;

	fs = s_rate;
	while (buffer_size < fs * 2.7f) {
	  buffer_size *= 2;
	}
	buffer = calloc(buffer_size, sizeof(LADSPA_Data));
	buffer_mask = buffer_size - 1;
	buffer_pos = 0;
	last_ll = 0.0f;
	last_cl = 0.0f;
	last_rl = 0.0f;
	last_ld = 0.0f;
	last_cd = 0.0f;
	last_rd = 0.0f;

	filters = malloc(2 * sizeof(biquad));

	plugin_data->buffer = buffer;
	plugin_data->buffer_mask = buffer_mask;
	plugin_data->buffer_pos = buffer_pos;
	plugin_data->filters = filters;
	plugin_data->fs = fs;
	plugin_data->last_cd = last_cd;
	plugin_data->last_cl = last_cl;
	plugin_data->last_ld = last_ld;
	plugin_data->last_ll = last_ll;
	plugin_data->last_rd = last_rd;
	plugin_data->last_rl = last_rl;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runLcrDelay(LADSPA_Handle instance, unsigned long sample_count) {
	LcrDelay *plugin_data = (LcrDelay *)instance;

	/* L delay (ms) (float value) */
	const LADSPA_Data ldel = *(plugin_data->ldel);

	/* L level (float value) */
	const LADSPA_Data llev = *(plugin_data->llev);

	/* C delay (ms) (float value) */
	const LADSPA_Data cdel = *(plugin_data->cdel);

	/* C level (float value) */
	const LADSPA_Data clev = *(plugin_data->clev);

	/* R delay (ms) (float value) */
	const LADSPA_Data rdel = *(plugin_data->rdel);

	/* R level (float value) */
	const LADSPA_Data rlev = *(plugin_data->rlev);

	/* Feedback (float value) */
	const LADSPA_Data feedback = *(plugin_data->feedback);

	/* High damp (%) (float value) */
	const LADSPA_Data high_d = *(plugin_data->high_d);

	/* Low damp (%) (float value) */
	const LADSPA_Data low_d = *(plugin_data->low_d);

	/* Spread (float value) */
	const LADSPA_Data spread = *(plugin_data->spread);

	/* Dry/Wet level (float value) */
	const LADSPA_Data wet = *(plugin_data->wet);

	/* L input (array of floats of length sample_count) */
	const LADSPA_Data * const in_l = plugin_data->in_l;

	/* R input (array of floats of length sample_count) */
	const LADSPA_Data * const in_r = plugin_data->in_r;

	/* L output (array of floats of length sample_count) */
	LADSPA_Data * const out_l = plugin_data->out_l;

	/* R output (array of floats of length sample_count) */
	LADSPA_Data * const out_r = plugin_data->out_r;
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	unsigned int buffer_pos = plugin_data->buffer_pos;
	biquad * filters = plugin_data->filters;
	float fs = plugin_data->fs;
	float last_cd = plugin_data->last_cd;
	float last_cl = plugin_data->last_cl;
	float last_ld = plugin_data->last_ld;
	float last_ll = plugin_data->last_ll;
	float last_rd = plugin_data->last_rd;
	float last_rl = plugin_data->last_rl;

#line 58 "lcr_delay_1436.xml"
	unsigned long pos;
	const float sc_r = 1.0f / (float)sample_count;
	const float spr_t = 0.5f + spread * 0.01f;
	const float spr_o = 0.5f - spread * 0.01f;
	float fb = feedback * 0.01f;
	float ll, cl, rl, ld, cd, rd;
	float ll_d, cl_d, rl_d, ld_d, cd_d, rd_d;
	float left, right;
	float fbs; /* Feedback signal */

	if (fb < -0.99f) {
	  fb = -0.99f;
	} else if (fb > 0.99f) {
	  fb = 0.99f;
	}

	ls_set_params(filters, fs * 0.0001f * powf(2.0f, low_d * 0.12f),
	              -0.5f * low_d, 0.5f, fs);
	hs_set_params(filters + 1, fs * (0.41f - 0.0001f *
	              powf(2.0f, high_d * 0.12f)), -70.0f, 0.9f, fs);

	ll = last_ll;                                /* Start value of Left Level */
	ll_d = (llev * 0.01f - last_ll) * sc_r;         /* Delta for Left Level */
	cl = last_cl;
	cl_d = (clev * 0.01f - last_cl) * sc_r;
	rl = last_rl;
	rl_d = (rlev * 0.01f - last_rl) * sc_r;

	ld = last_ld;
	ld_d = (ldel * fs * 0.001f - last_ld) * sc_r;
	cd = last_cd;
	cd_d = (cdel * fs * 0.001f - last_cd) * sc_r;
	rd = last_rd;
	rd_d = (rdel * fs * 0.001f - last_rd) * sc_r;

	for (pos = 0; pos < sample_count; pos++) {
	  /* Increment linear interpolators */
	  ll += ll_d;
	  rl += rl_d;
	  cl += cl_d;
	  ld += ld_d;
	  rd += rd_d;
	  cd += cd_d;

	  /* Write input into delay line */
	  buffer[buffer_pos] = in_l[pos] + in_r[pos];
	  /* Add feedback, must be done afterwards for case where C delay = 0 */
	  fbs = buffer[(buffer_pos - f_round(cd)) & buffer_mask] * fb;
	  fbs = flush_to_zero(fbs);
	  fbs = biquad_run(filters, fbs);
	  fbs = biquad_run(filters + 1, fbs);
	  buffer[buffer_pos] += fbs;

	  /* Outputs from left and right delay beffers + centre mix */
	  left  = buffer[(buffer_pos - f_round(ld)) & buffer_mask] * ll +
	          buffer[(buffer_pos - f_round(cd)) & buffer_mask] * cl;
	  right = buffer[(buffer_pos - f_round(rd)) & buffer_mask] * rl +
	          buffer[(buffer_pos - f_round(cd)) & buffer_mask] * cl;

	  /* Left and right channel outs */
	  buffer_write(out_l[pos], in_l[pos] * (1.0f - wet) +
	                  (left * spr_t + right * spr_o) * wet);
	  buffer_write(out_r[pos], in_r[pos] * (1.0f - wet) +
	                  (left * spr_o + right * spr_t) * wet);

	  buffer_pos = (buffer_pos + 1) & buffer_mask;
	}

	plugin_data->last_ll = ll;
	plugin_data->last_cl = cl;
	plugin_data->last_rl = rl;
	plugin_data->last_ld = ld;
	plugin_data->last_cd = cd;
	plugin_data->last_rd = rd;
	plugin_data->buffer_pos = buffer_pos;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainLcrDelay(LADSPA_Handle instance, LADSPA_Data gain) {
	((LcrDelay *)instance)->run_adding_gain = gain;
}

static void runAddingLcrDelay(LADSPA_Handle instance, unsigned long sample_count) {
	LcrDelay *plugin_data = (LcrDelay *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* L delay (ms) (float value) */
	const LADSPA_Data ldel = *(plugin_data->ldel);

	/* L level (float value) */
	const LADSPA_Data llev = *(plugin_data->llev);

	/* C delay (ms) (float value) */
	const LADSPA_Data cdel = *(plugin_data->cdel);

	/* C level (float value) */
	const LADSPA_Data clev = *(plugin_data->clev);

	/* R delay (ms) (float value) */
	const LADSPA_Data rdel = *(plugin_data->rdel);

	/* R level (float value) */
	const LADSPA_Data rlev = *(plugin_data->rlev);

	/* Feedback (float value) */
	const LADSPA_Data feedback = *(plugin_data->feedback);

	/* High damp (%) (float value) */
	const LADSPA_Data high_d = *(plugin_data->high_d);

	/* Low damp (%) (float value) */
	const LADSPA_Data low_d = *(plugin_data->low_d);

	/* Spread (float value) */
	const LADSPA_Data spread = *(plugin_data->spread);

	/* Dry/Wet level (float value) */
	const LADSPA_Data wet = *(plugin_data->wet);

	/* L input (array of floats of length sample_count) */
	const LADSPA_Data * const in_l = plugin_data->in_l;

	/* R input (array of floats of length sample_count) */
	const LADSPA_Data * const in_r = plugin_data->in_r;

	/* L output (array of floats of length sample_count) */
	LADSPA_Data * const out_l = plugin_data->out_l;

	/* R output (array of floats of length sample_count) */
	LADSPA_Data * const out_r = plugin_data->out_r;
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	unsigned int buffer_pos = plugin_data->buffer_pos;
	biquad * filters = plugin_data->filters;
	float fs = plugin_data->fs;
	float last_cd = plugin_data->last_cd;
	float last_cl = plugin_data->last_cl;
	float last_ld = plugin_data->last_ld;
	float last_ll = plugin_data->last_ll;
	float last_rd = plugin_data->last_rd;
	float last_rl = plugin_data->last_rl;

#line 58 "lcr_delay_1436.xml"
	unsigned long pos;
	const float sc_r = 1.0f / (float)sample_count;
	const float spr_t = 0.5f + spread * 0.01f;
	const float spr_o = 0.5f - spread * 0.01f;
	float fb = feedback * 0.01f;
	float ll, cl, rl, ld, cd, rd;
	float ll_d, cl_d, rl_d, ld_d, cd_d, rd_d;
	float left, right;
	float fbs; /* Feedback signal */

	if (fb < -0.99f) {
	  fb = -0.99f;
	} else if (fb > 0.99f) {
	  fb = 0.99f;
	}

	ls_set_params(filters, fs * 0.0001f * powf(2.0f, low_d * 0.12f),
	              -0.5f * low_d, 0.5f, fs);
	hs_set_params(filters + 1, fs * (0.41f - 0.0001f *
	              powf(2.0f, high_d * 0.12f)), -70.0f, 0.9f, fs);

	ll = last_ll;                                /* Start value of Left Level */
	ll_d = (llev * 0.01f - last_ll) * sc_r;         /* Delta for Left Level */
	cl = last_cl;
	cl_d = (clev * 0.01f - last_cl) * sc_r;
	rl = last_rl;
	rl_d = (rlev * 0.01f - last_rl) * sc_r;

	ld = last_ld;
	ld_d = (ldel * fs * 0.001f - last_ld) * sc_r;
	cd = last_cd;
	cd_d = (cdel * fs * 0.001f - last_cd) * sc_r;
	rd = last_rd;
	rd_d = (rdel * fs * 0.001f - last_rd) * sc_r;

	for (pos = 0; pos < sample_count; pos++) {
	  /* Increment linear interpolators */
	  ll += ll_d;
	  rl += rl_d;
	  cl += cl_d;
	  ld += ld_d;
	  rd += rd_d;
	  cd += cd_d;

	  /* Write input into delay line */
	  buffer[buffer_pos] = in_l[pos] + in_r[pos];
	  /* Add feedback, must be done afterwards for case where C delay = 0 */
	  fbs = buffer[(buffer_pos - f_round(cd)) & buffer_mask] * fb;
	  fbs = flush_to_zero(fbs);
	  fbs = biquad_run(filters, fbs);
	  fbs = biquad_run(filters + 1, fbs);
	  buffer[buffer_pos] += fbs;

	  /* Outputs from left and right delay beffers + centre mix */
	  left  = buffer[(buffer_pos - f_round(ld)) & buffer_mask] * ll +
	          buffer[(buffer_pos - f_round(cd)) & buffer_mask] * cl;
	  right = buffer[(buffer_pos - f_round(rd)) & buffer_mask] * rl +
	          buffer[(buffer_pos - f_round(cd)) & buffer_mask] * cl;

	  /* Left and right channel outs */
	  buffer_write(out_l[pos], in_l[pos] * (1.0f - wet) +
	                  (left * spr_t + right * spr_o) * wet);
	  buffer_write(out_r[pos], in_r[pos] * (1.0f - wet) +
	                  (left * spr_o + right * spr_t) * wet);

	  buffer_pos = (buffer_pos + 1) & buffer_mask;
	}

	plugin_data->last_ll = ll;
	plugin_data->last_cl = cl;
	plugin_data->last_rl = rl;
	plugin_data->last_ld = ld;
	plugin_data->last_cd = cd;
	plugin_data->last_rd = rd;
	plugin_data->buffer_pos = buffer_pos;
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


	lcrDelayDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (lcrDelayDescriptor) {
		lcrDelayDescriptor->UniqueID = 1436;
		lcrDelayDescriptor->Label = "lcrDelay";
		lcrDelayDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		lcrDelayDescriptor->Name =
		 D_("L/C/R Delay");
		lcrDelayDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		lcrDelayDescriptor->Copyright =
		 "GPL";
		lcrDelayDescriptor->PortCount = 15;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(15,
		 sizeof(LADSPA_PortDescriptor));
		lcrDelayDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(15,
		 sizeof(LADSPA_PortRangeHint));
		lcrDelayDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(15, sizeof(char*));
		lcrDelayDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for L delay (ms) */
		port_descriptors[LCRDELAY_LDEL] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LCRDELAY_LDEL] =
		 D_("L delay (ms)");
		port_range_hints[LCRDELAY_LDEL].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[LCRDELAY_LDEL].LowerBound = 0;
		port_range_hints[LCRDELAY_LDEL].UpperBound = 2700;

		/* Parameters for L level */
		port_descriptors[LCRDELAY_LLEV] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LCRDELAY_LLEV] =
		 D_("L level");
		port_range_hints[LCRDELAY_LLEV].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[LCRDELAY_LLEV].LowerBound = 0;
		port_range_hints[LCRDELAY_LLEV].UpperBound = 50;

		/* Parameters for C delay (ms) */
		port_descriptors[LCRDELAY_CDEL] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LCRDELAY_CDEL] =
		 D_("C delay (ms)");
		port_range_hints[LCRDELAY_CDEL].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[LCRDELAY_CDEL].LowerBound = 0;
		port_range_hints[LCRDELAY_CDEL].UpperBound = 2700;

		/* Parameters for C level */
		port_descriptors[LCRDELAY_CLEV] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LCRDELAY_CLEV] =
		 D_("C level");
		port_range_hints[LCRDELAY_CLEV].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[LCRDELAY_CLEV].LowerBound = 0;
		port_range_hints[LCRDELAY_CLEV].UpperBound = 50;

		/* Parameters for R delay (ms) */
		port_descriptors[LCRDELAY_RDEL] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LCRDELAY_RDEL] =
		 D_("R delay (ms)");
		port_range_hints[LCRDELAY_RDEL].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[LCRDELAY_RDEL].LowerBound = 0;
		port_range_hints[LCRDELAY_RDEL].UpperBound = 2700;

		/* Parameters for R level */
		port_descriptors[LCRDELAY_RLEV] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LCRDELAY_RLEV] =
		 D_("R level");
		port_range_hints[LCRDELAY_RLEV].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[LCRDELAY_RLEV].LowerBound = 0;
		port_range_hints[LCRDELAY_RLEV].UpperBound = 50;

		/* Parameters for Feedback */
		port_descriptors[LCRDELAY_FEEDBACK] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LCRDELAY_FEEDBACK] =
		 D_("Feedback");
		port_range_hints[LCRDELAY_FEEDBACK].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[LCRDELAY_FEEDBACK].LowerBound = -100;
		port_range_hints[LCRDELAY_FEEDBACK].UpperBound = 100;

		/* Parameters for High damp (%) */
		port_descriptors[LCRDELAY_HIGH_D] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LCRDELAY_HIGH_D] =
		 D_("High damp (%)");
		port_range_hints[LCRDELAY_HIGH_D].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[LCRDELAY_HIGH_D].LowerBound = 0;
		port_range_hints[LCRDELAY_HIGH_D].UpperBound = 100;

		/* Parameters for Low damp (%) */
		port_descriptors[LCRDELAY_LOW_D] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LCRDELAY_LOW_D] =
		 D_("Low damp (%)");
		port_range_hints[LCRDELAY_LOW_D].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[LCRDELAY_LOW_D].LowerBound = 0;
		port_range_hints[LCRDELAY_LOW_D].UpperBound = 100;

		/* Parameters for Spread */
		port_descriptors[LCRDELAY_SPREAD] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LCRDELAY_SPREAD] =
		 D_("Spread");
		port_range_hints[LCRDELAY_SPREAD].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[LCRDELAY_SPREAD].LowerBound = 0;
		port_range_hints[LCRDELAY_SPREAD].UpperBound = 50;

		/* Parameters for Dry/Wet level */
		port_descriptors[LCRDELAY_WET] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LCRDELAY_WET] =
		 D_("Dry/Wet level");
		port_range_hints[LCRDELAY_WET].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[LCRDELAY_WET].LowerBound = 0;
		port_range_hints[LCRDELAY_WET].UpperBound = 1;

		/* Parameters for L input */
		port_descriptors[LCRDELAY_IN_L] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[LCRDELAY_IN_L] =
		 D_("L input");
		port_range_hints[LCRDELAY_IN_L].HintDescriptor = 0;

		/* Parameters for R input */
		port_descriptors[LCRDELAY_IN_R] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[LCRDELAY_IN_R] =
		 D_("R input");
		port_range_hints[LCRDELAY_IN_R].HintDescriptor = 0;

		/* Parameters for L output */
		port_descriptors[LCRDELAY_OUT_L] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[LCRDELAY_OUT_L] =
		 D_("L output");
		port_range_hints[LCRDELAY_OUT_L].HintDescriptor = 0;

		/* Parameters for R output */
		port_descriptors[LCRDELAY_OUT_R] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[LCRDELAY_OUT_R] =
		 D_("R output");
		port_range_hints[LCRDELAY_OUT_R].HintDescriptor = 0;

		lcrDelayDescriptor->activate = activateLcrDelay;
		lcrDelayDescriptor->cleanup = cleanupLcrDelay;
		lcrDelayDescriptor->connect_port = connectPortLcrDelay;
		lcrDelayDescriptor->deactivate = NULL;
		lcrDelayDescriptor->instantiate = instantiateLcrDelay;
		lcrDelayDescriptor->run = runLcrDelay;
		lcrDelayDescriptor->run_adding = runAddingLcrDelay;
		lcrDelayDescriptor->set_run_adding_gain = setRunAddingGainLcrDelay;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (lcrDelayDescriptor) {
		free((LADSPA_PortDescriptor *)lcrDelayDescriptor->PortDescriptors);
		free((char **)lcrDelayDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)lcrDelayDescriptor->PortRangeHints);
		free(lcrDelayDescriptor);
	}

}
