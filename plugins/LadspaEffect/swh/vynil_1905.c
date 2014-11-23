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

#line 10 "vynil_1905.xml"

#include <stdlib.h>
#include <limits.h>

#include "ladspa-util.h"
#include "util/biquad.h"

#define BUF_LEN 0.1
#define CLICK_BUF_SIZE 4096

#define df(x) ((sinf(x) + 1.0f) * 0.5f)

inline static float noise();
inline static float noise()
{
   static unsigned int randSeed = 23;
   randSeed = (randSeed * 196314165) + 907633515;
   return randSeed / (float)INT_MAX - 1.0f;
}

#define VYNIL_YEAR                     0
#define VYNIL_RPM                      1
#define VYNIL_WARP                     2
#define VYNIL_CLICK                    3
#define VYNIL_WEAR                     4
#define VYNIL_IN_L                     5
#define VYNIL_IN_R                     6
#define VYNIL_OUT_L                    7
#define VYNIL_OUT_R                    8

static LADSPA_Descriptor *vynilDescriptor = NULL;

typedef struct {
	LADSPA_Data *year;
	LADSPA_Data *rpm;
	LADSPA_Data *warp;
	LADSPA_Data *click;
	LADSPA_Data *wear;
	LADSPA_Data *in_l;
	LADSPA_Data *in_r;
	LADSPA_Data *out_l;
	LADSPA_Data *out_r;
	LADSPA_Data *buffer_m;
	unsigned int buffer_mask;
	unsigned int buffer_pos;
	LADSPA_Data *buffer_s;
	LADSPA_Data *click_buffer;
	fixp16       click_buffer_omega;
	fixp16       click_buffer_pos;
	float        click_gain;
	float        def;
	float        def_target;
	float        fs;
	biquad *     highp;
	biquad *     lowp_m;
	biquad *     lowp_s;
	biquad *     noise_filt;
	float        phi;
	unsigned int sample_cnt;
	LADSPA_Data run_adding_gain;
} Vynil;

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
		return vynilDescriptor;
	default:
		return NULL;
	}
}

static void activateVynil(LADSPA_Handle instance) {
	Vynil *plugin_data = (Vynil *)instance;
	LADSPA_Data *buffer_m = plugin_data->buffer_m;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	unsigned int buffer_pos = plugin_data->buffer_pos;
	LADSPA_Data *buffer_s = plugin_data->buffer_s;
	LADSPA_Data *click_buffer = plugin_data->click_buffer;
	fixp16 click_buffer_omega = plugin_data->click_buffer_omega;
	fixp16 click_buffer_pos = plugin_data->click_buffer_pos;
	float click_gain = plugin_data->click_gain;
	float def = plugin_data->def;
	float def_target = plugin_data->def_target;
	float fs = plugin_data->fs;
	biquad *highp = plugin_data->highp;
	biquad *lowp_m = plugin_data->lowp_m;
	biquad *lowp_s = plugin_data->lowp_s;
	biquad *noise_filt = plugin_data->noise_filt;
	float phi = plugin_data->phi;
	unsigned int sample_cnt = plugin_data->sample_cnt;
#line 75 "vynil_1905.xml"
	memset(buffer_m, 0, sizeof(LADSPA_Data) * (buffer_mask + 1));
	memset(buffer_s, 0, sizeof(LADSPA_Data) * (buffer_mask + 1));
	buffer_pos = 0;
	click_buffer_pos.all = 0;
	click_buffer_omega.all = 0;
	click_gain = 0;
	phi = 0.0f;

	lp_set_params(lowp_m, 16000.0, 0.5, fs);
	lp_set_params(lowp_s, 16000.0, 0.5, fs);
	lp_set_params(highp, 10.0, 0.5, fs);
	lp_set_params(noise_filt, 1000.0, 0.5, fs);
	plugin_data->buffer_m = buffer_m;
	plugin_data->buffer_mask = buffer_mask;
	plugin_data->buffer_pos = buffer_pos;
	plugin_data->buffer_s = buffer_s;
	plugin_data->click_buffer = click_buffer;
	plugin_data->click_buffer_omega = click_buffer_omega;
	plugin_data->click_buffer_pos = click_buffer_pos;
	plugin_data->click_gain = click_gain;
	plugin_data->def = def;
	plugin_data->def_target = def_target;
	plugin_data->fs = fs;
	plugin_data->highp = highp;
	plugin_data->lowp_m = lowp_m;
	plugin_data->lowp_s = lowp_s;
	plugin_data->noise_filt = noise_filt;
	plugin_data->phi = phi;
	plugin_data->sample_cnt = sample_cnt;

}

static void cleanupVynil(LADSPA_Handle instance) {
#line 179 "vynil_1905.xml"
	Vynil *plugin_data = (Vynil *)instance;
	free(plugin_data->buffer_m);
	free(plugin_data->buffer_s);
	free(plugin_data->click_buffer);
	free(plugin_data->lowp_m);
	free(plugin_data->lowp_s);
	free(plugin_data->noise_filt);
	free(instance);
}

static void connectPortVynil(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Vynil *plugin;

	plugin = (Vynil *)instance;
	switch (port) {
	case VYNIL_YEAR:
		plugin->year = data;
		break;
	case VYNIL_RPM:
		plugin->rpm = data;
		break;
	case VYNIL_WARP:
		plugin->warp = data;
		break;
	case VYNIL_CLICK:
		plugin->click = data;
		break;
	case VYNIL_WEAR:
		plugin->wear = data;
		break;
	case VYNIL_IN_L:
		plugin->in_l = data;
		break;
	case VYNIL_IN_R:
		plugin->in_r = data;
		break;
	case VYNIL_OUT_L:
		plugin->out_l = data;
		break;
	case VYNIL_OUT_R:
		plugin->out_r = data;
		break;
	}
}

static LADSPA_Handle instantiateVynil(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Vynil *plugin_data = (Vynil *)malloc(sizeof(Vynil));
	LADSPA_Data *buffer_m = NULL;
	unsigned int buffer_mask;
	unsigned int buffer_pos;
	LADSPA_Data *buffer_s = NULL;
	LADSPA_Data *click_buffer = NULL;
	fixp16 click_buffer_omega;
	fixp16 click_buffer_pos;
	float click_gain;
	float def;
	float def_target;
	float fs;
	biquad *highp = NULL;
	biquad *lowp_m = NULL;
	biquad *lowp_s = NULL;
	biquad *noise_filt = NULL;
	float phi;
	unsigned int sample_cnt;

#line 37 "vynil_1905.xml"
	unsigned int i;
	unsigned int buffer_size;

	fs = (float)s_rate;
	buffer_size = 4096;
	while (buffer_size < s_rate * BUF_LEN) {
	  buffer_size *= 2;
	}
	buffer_m = malloc(sizeof(LADSPA_Data) * buffer_size);
	buffer_s = malloc(sizeof(LADSPA_Data) * buffer_size);
	buffer_mask = buffer_size - 1;
	buffer_pos = 0;
	click_gain = 0;
	phi = 0.0f; /* Angular phase */

	click_buffer = malloc(sizeof(LADSPA_Data) * CLICK_BUF_SIZE);
	for (i=0; i<CLICK_BUF_SIZE; i++) {
	  if (i<CLICK_BUF_SIZE / 2) {
	    click_buffer[i] = (double)i / (double)(CLICK_BUF_SIZE / 2);
	    click_buffer[i] *= click_buffer[i];
	    click_buffer[i] *= click_buffer[i];
	    click_buffer[i] *= click_buffer[i];
	  } else {
	    click_buffer[i] = click_buffer[CLICK_BUF_SIZE - i];
	  }
	}

	sample_cnt = 0;
	def = 0.0f;
	def_target = 0.0f;

	lowp_m = calloc(sizeof(biquad), 1);
	lowp_s = calloc(sizeof(biquad), 1);
	highp = calloc(sizeof(biquad), 1);
	noise_filt = calloc(sizeof(biquad), 1);

	plugin_data->buffer_m = buffer_m;
	plugin_data->buffer_mask = buffer_mask;
	plugin_data->buffer_pos = buffer_pos;
	plugin_data->buffer_s = buffer_s;
	plugin_data->click_buffer = click_buffer;
	plugin_data->click_buffer_omega = click_buffer_omega;
	plugin_data->click_buffer_pos = click_buffer_pos;
	plugin_data->click_gain = click_gain;
	plugin_data->def = def;
	plugin_data->def_target = def_target;
	plugin_data->fs = fs;
	plugin_data->highp = highp;
	plugin_data->lowp_m = lowp_m;
	plugin_data->lowp_s = lowp_s;
	plugin_data->noise_filt = noise_filt;
	plugin_data->phi = phi;
	plugin_data->sample_cnt = sample_cnt;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runVynil(LADSPA_Handle instance, unsigned long sample_count) {
	Vynil *plugin_data = (Vynil *)instance;

	/* Year (float value) */
	const LADSPA_Data year = *(plugin_data->year);

	/* RPM (float value) */
	const LADSPA_Data rpm = *(plugin_data->rpm);

	/* Surface warping (float value) */
	const LADSPA_Data warp = *(plugin_data->warp);

	/* Crackle (float value) */
	const LADSPA_Data click = *(plugin_data->click);

	/* Wear (float value) */
	const LADSPA_Data wear = *(plugin_data->wear);

	/* Input L (array of floats of length sample_count) */
	const LADSPA_Data * const in_l = plugin_data->in_l;

	/* Input R (array of floats of length sample_count) */
	const LADSPA_Data * const in_r = plugin_data->in_r;

	/* Output L (array of floats of length sample_count) */
	LADSPA_Data * const out_l = plugin_data->out_l;

	/* Output R (array of floats of length sample_count) */
	LADSPA_Data * const out_r = plugin_data->out_r;
	LADSPA_Data * buffer_m = plugin_data->buffer_m;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	unsigned int buffer_pos = plugin_data->buffer_pos;
	LADSPA_Data * buffer_s = plugin_data->buffer_s;
	LADSPA_Data * click_buffer = plugin_data->click_buffer;
	fixp16 click_buffer_omega = plugin_data->click_buffer_omega;
	fixp16 click_buffer_pos = plugin_data->click_buffer_pos;
	float click_gain = plugin_data->click_gain;
	float def = plugin_data->def;
	float def_target = plugin_data->def_target;
	float fs = plugin_data->fs;
	biquad * highp = plugin_data->highp;
	biquad * lowp_m = plugin_data->lowp_m;
	biquad * lowp_s = plugin_data->lowp_s;
	biquad * noise_filt = plugin_data->noise_filt;
	float phi = plugin_data->phi;
	unsigned int sample_cnt = plugin_data->sample_cnt;

#line 90 "vynil_1905.xml"
	unsigned long pos;
	float deflec = def;
	float deflec_target = def_target;
	float src_m, src_s;

	/* angular velocity of platter * 16 */
	const float omega = 960.0f / (rpm * fs);
	const float age = (2000 - year) * 0.01f;
	const unsigned int click_prob = (age*age*(float)RAND_MAX)/10 + click * 0.02 * RAND_MAX;
	const float noise_amp = (click + wear * 0.3f) * 0.12f + (1993.0f - year) * 0.0031f;
	const float bandwidth = (year - 1880.0f) * (rpm * 1.9f);
	const float noise_bandwidth = bandwidth * (0.25 - wear * 0.02) + click * 200.0 + 300.0;
	const float stereo = f_clamp((year - 1940.0f) * 0.02f, 0.0f, 1.0f);
	const float wrap_gain = age * 3.1f + 0.05f;
	const float wrap_bias = age * 0.1f;

	lp_set_params(lowp_m, bandwidth * (1.0 - wear * 0.86), 2.0, fs);
	lp_set_params(lowp_s, bandwidth * (1.0 - wear * 0.89), 2.0, fs);
	hp_set_params(highp, (2000-year) * 8.0, 1.5, fs);
	lp_set_params(noise_filt, noise_bandwidth, 4.0 + wear * 2.0, fs);

	for (pos = 0; pos < sample_count; pos++) {
	  unsigned int o1, o2;
	  float ofs;

	  if ((sample_cnt & 15) == 0) {
	    const float ang = phi * 2.0f * M_PI;
	    const float w = warp * (2000.0f - year) * 0.01f;
	    deflec_target = w*df(ang)*0.5f + w*w*df(2.0f*ang)*0.31f +
	                       w*w*w*df(3.0f*ang)*0.129f;
	    phi += omega;
	    while (phi > 1.0f) {
	      phi -= 1.0f;
	    }
	    if ((unsigned int)rand() < click_prob) {
	      click_buffer_omega.all = ((rand() >> 6) + 1000) * rpm;
	      click_gain = noise_amp * 5.0f * noise();
	    }
	  }
	  deflec = deflec * 0.1f + deflec_target * 0.9f;

	  /* matrix into mid_side representation (this is roughly what stereo
	   * LPs do) */
	  buffer_m[buffer_pos] = in_l[pos] + in_r[pos];
	  buffer_s[buffer_pos] = in_l[pos] - in_r[pos];

	  /* cacluate the effects of the surface warping */
	  ofs = fs * 0.009f * deflec;
	  o1 = f_round(floorf(ofs));
	  o2 = f_round(ceilf(ofs));
	  ofs -= o1;
	  src_m = LIN_INTERP(ofs, buffer_m[(buffer_pos - o1 - 1) & buffer_mask], buffer_m[(buffer_pos - o2 - 1) & buffer_mask]);
	  src_s = LIN_INTERP(ofs, buffer_s[(buffer_pos - o1 - 1) & buffer_mask], buffer_s[(buffer_pos - o2 - 1) & buffer_mask]);

	  src_m = biquad_run(lowp_m, src_m + click_buffer[click_buffer_pos.part.in & (CLICK_BUF_SIZE - 1)] * click_gain);

	  /* waveshaper */
	  src_m = LIN_INTERP(age, src_m, sinf(src_m * wrap_gain + wrap_bias));

	  /* output highpass */
	  src_m = biquad_run(highp, src_m) + biquad_run(noise_filt, noise()) * noise_amp + click_buffer[click_buffer_pos.part.in & (CLICK_BUF_SIZE - 1)] * click_gain * 0.5f;

	  /* stereo seperation filter */
	  src_s = biquad_run(lowp_s, src_s) * stereo;

	  buffer_write(out_l[pos], (src_s + src_m) * 0.5f);
	  buffer_write(out_r[pos], (src_m - src_s) * 0.5f);

	  /* roll buffer indexes */
	  buffer_pos = (buffer_pos + 1) & buffer_mask;
	  click_buffer_pos.all += click_buffer_omega.all;
	  if (click_buffer_pos.part.in >= CLICK_BUF_SIZE) {
	    click_buffer_pos.all = 0;
	    click_buffer_omega.all = 0;
	  }
	  sample_cnt++;
	}

	plugin_data->buffer_pos = buffer_pos;
	plugin_data->click_buffer_pos = click_buffer_pos;
	plugin_data->click_buffer_omega = click_buffer_omega;
	plugin_data->click_gain = click_gain;
	plugin_data->sample_cnt = sample_cnt;
	plugin_data->def_target = deflec_target;
	plugin_data->def = deflec;
	plugin_data->phi = phi;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainVynil(LADSPA_Handle instance, LADSPA_Data gain) {
	((Vynil *)instance)->run_adding_gain = gain;
}

static void runAddingVynil(LADSPA_Handle instance, unsigned long sample_count) {
	Vynil *plugin_data = (Vynil *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Year (float value) */
	const LADSPA_Data year = *(plugin_data->year);

	/* RPM (float value) */
	const LADSPA_Data rpm = *(plugin_data->rpm);

	/* Surface warping (float value) */
	const LADSPA_Data warp = *(plugin_data->warp);

	/* Crackle (float value) */
	const LADSPA_Data click = *(plugin_data->click);

	/* Wear (float value) */
	const LADSPA_Data wear = *(plugin_data->wear);

	/* Input L (array of floats of length sample_count) */
	const LADSPA_Data * const in_l = plugin_data->in_l;

	/* Input R (array of floats of length sample_count) */
	const LADSPA_Data * const in_r = plugin_data->in_r;

	/* Output L (array of floats of length sample_count) */
	LADSPA_Data * const out_l = plugin_data->out_l;

	/* Output R (array of floats of length sample_count) */
	LADSPA_Data * const out_r = plugin_data->out_r;
	LADSPA_Data * buffer_m = plugin_data->buffer_m;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	unsigned int buffer_pos = plugin_data->buffer_pos;
	LADSPA_Data * buffer_s = plugin_data->buffer_s;
	LADSPA_Data * click_buffer = plugin_data->click_buffer;
	fixp16 click_buffer_omega = plugin_data->click_buffer_omega;
	fixp16 click_buffer_pos = plugin_data->click_buffer_pos;
	float click_gain = plugin_data->click_gain;
	float def = plugin_data->def;
	float def_target = plugin_data->def_target;
	float fs = plugin_data->fs;
	biquad * highp = plugin_data->highp;
	biquad * lowp_m = plugin_data->lowp_m;
	biquad * lowp_s = plugin_data->lowp_s;
	biquad * noise_filt = plugin_data->noise_filt;
	float phi = plugin_data->phi;
	unsigned int sample_cnt = plugin_data->sample_cnt;

#line 90 "vynil_1905.xml"
	unsigned long pos;
	float deflec = def;
	float deflec_target = def_target;
	float src_m, src_s;

	/* angular velocity of platter * 16 */
	const float omega = 960.0f / (rpm * fs);
	const float age = (2000 - year) * 0.01f;
	const unsigned int click_prob = (age*age*(float)RAND_MAX)/10 + click * 0.02 * RAND_MAX;
	const float noise_amp = (click + wear * 0.3f) * 0.12f + (1993.0f - year) * 0.0031f;
	const float bandwidth = (year - 1880.0f) * (rpm * 1.9f);
	const float noise_bandwidth = bandwidth * (0.25 - wear * 0.02) + click * 200.0 + 300.0;
	const float stereo = f_clamp((year - 1940.0f) * 0.02f, 0.0f, 1.0f);
	const float wrap_gain = age * 3.1f + 0.05f;
	const float wrap_bias = age * 0.1f;

	lp_set_params(lowp_m, bandwidth * (1.0 - wear * 0.86), 2.0, fs);
	lp_set_params(lowp_s, bandwidth * (1.0 - wear * 0.89), 2.0, fs);
	hp_set_params(highp, (2000-year) * 8.0, 1.5, fs);
	lp_set_params(noise_filt, noise_bandwidth, 4.0 + wear * 2.0, fs);

	for (pos = 0; pos < sample_count; pos++) {
	  unsigned int o1, o2;
	  float ofs;

	  if ((sample_cnt & 15) == 0) {
	    const float ang = phi * 2.0f * M_PI;
	    const float w = warp * (2000.0f - year) * 0.01f;
	    deflec_target = w*df(ang)*0.5f + w*w*df(2.0f*ang)*0.31f +
	                       w*w*w*df(3.0f*ang)*0.129f;
	    phi += omega;
	    while (phi > 1.0f) {
	      phi -= 1.0f;
	    }
	    if ((unsigned int)rand() < click_prob) {
	      click_buffer_omega.all = ((rand() >> 6) + 1000) * rpm;
	      click_gain = noise_amp * 5.0f * noise();
	    }
	  }
	  deflec = deflec * 0.1f + deflec_target * 0.9f;

	  /* matrix into mid_side representation (this is roughly what stereo
	   * LPs do) */
	  buffer_m[buffer_pos] = in_l[pos] + in_r[pos];
	  buffer_s[buffer_pos] = in_l[pos] - in_r[pos];

	  /* cacluate the effects of the surface warping */
	  ofs = fs * 0.009f * deflec;
	  o1 = f_round(floorf(ofs));
	  o2 = f_round(ceilf(ofs));
	  ofs -= o1;
	  src_m = LIN_INTERP(ofs, buffer_m[(buffer_pos - o1 - 1) & buffer_mask], buffer_m[(buffer_pos - o2 - 1) & buffer_mask]);
	  src_s = LIN_INTERP(ofs, buffer_s[(buffer_pos - o1 - 1) & buffer_mask], buffer_s[(buffer_pos - o2 - 1) & buffer_mask]);

	  src_m = biquad_run(lowp_m, src_m + click_buffer[click_buffer_pos.part.in & (CLICK_BUF_SIZE - 1)] * click_gain);

	  /* waveshaper */
	  src_m = LIN_INTERP(age, src_m, sinf(src_m * wrap_gain + wrap_bias));

	  /* output highpass */
	  src_m = biquad_run(highp, src_m) + biquad_run(noise_filt, noise()) * noise_amp + click_buffer[click_buffer_pos.part.in & (CLICK_BUF_SIZE - 1)] * click_gain * 0.5f;

	  /* stereo seperation filter */
	  src_s = biquad_run(lowp_s, src_s) * stereo;

	  buffer_write(out_l[pos], (src_s + src_m) * 0.5f);
	  buffer_write(out_r[pos], (src_m - src_s) * 0.5f);

	  /* roll buffer indexes */
	  buffer_pos = (buffer_pos + 1) & buffer_mask;
	  click_buffer_pos.all += click_buffer_omega.all;
	  if (click_buffer_pos.part.in >= CLICK_BUF_SIZE) {
	    click_buffer_pos.all = 0;
	    click_buffer_omega.all = 0;
	  }
	  sample_cnt++;
	}

	plugin_data->buffer_pos = buffer_pos;
	plugin_data->click_buffer_pos = click_buffer_pos;
	plugin_data->click_buffer_omega = click_buffer_omega;
	plugin_data->click_gain = click_gain;
	plugin_data->sample_cnt = sample_cnt;
	plugin_data->def_target = deflec_target;
	plugin_data->def = deflec;
	plugin_data->phi = phi;
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


	vynilDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (vynilDescriptor) {
		vynilDescriptor->UniqueID = 1905;
		vynilDescriptor->Label = "vynil";
		vynilDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		vynilDescriptor->Name =
		 D_("VyNil (Vinyl Effect)");
		vynilDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		vynilDescriptor->Copyright =
		 "GPL";
		vynilDescriptor->PortCount = 9;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(9,
		 sizeof(LADSPA_PortDescriptor));
		vynilDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(9,
		 sizeof(LADSPA_PortRangeHint));
		vynilDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(9, sizeof(char*));
		vynilDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Year */
		port_descriptors[VYNIL_YEAR] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VYNIL_YEAR] =
		 D_("Year");
		port_range_hints[VYNIL_YEAR].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MAXIMUM;
		port_range_hints[VYNIL_YEAR].LowerBound = 1900;
		port_range_hints[VYNIL_YEAR].UpperBound = 1990;

		/* Parameters for RPM */
		port_descriptors[VYNIL_RPM] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VYNIL_RPM] =
		 D_("RPM");
		port_range_hints[VYNIL_RPM].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MINIMUM;
		port_range_hints[VYNIL_RPM].LowerBound = 33;
		port_range_hints[VYNIL_RPM].UpperBound = 78;

		/* Parameters for Surface warping */
		port_descriptors[VYNIL_WARP] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VYNIL_WARP] =
		 D_("Surface warping");
		port_range_hints[VYNIL_WARP].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[VYNIL_WARP].LowerBound = 0.0;
		port_range_hints[VYNIL_WARP].UpperBound = 1.0;

		/* Parameters for Crackle */
		port_descriptors[VYNIL_CLICK] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VYNIL_CLICK] =
		 D_("Crackle");
		port_range_hints[VYNIL_CLICK].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[VYNIL_CLICK].LowerBound = 0.0;
		port_range_hints[VYNIL_CLICK].UpperBound = 1.0;

		/* Parameters for Wear */
		port_descriptors[VYNIL_WEAR] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VYNIL_WEAR] =
		 D_("Wear");
		port_range_hints[VYNIL_WEAR].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[VYNIL_WEAR].LowerBound = 0.0;
		port_range_hints[VYNIL_WEAR].UpperBound = 1.0;

		/* Parameters for Input L */
		port_descriptors[VYNIL_IN_L] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[VYNIL_IN_L] =
		 D_("Input L");
		port_range_hints[VYNIL_IN_L].HintDescriptor = 0;

		/* Parameters for Input R */
		port_descriptors[VYNIL_IN_R] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[VYNIL_IN_R] =
		 D_("Input R");
		port_range_hints[VYNIL_IN_R].HintDescriptor = 0;

		/* Parameters for Output L */
		port_descriptors[VYNIL_OUT_L] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[VYNIL_OUT_L] =
		 D_("Output L");
		port_range_hints[VYNIL_OUT_L].HintDescriptor = 0;

		/* Parameters for Output R */
		port_descriptors[VYNIL_OUT_R] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[VYNIL_OUT_R] =
		 D_("Output R");
		port_range_hints[VYNIL_OUT_R].HintDescriptor = 0;

		vynilDescriptor->activate = activateVynil;
		vynilDescriptor->cleanup = cleanupVynil;
		vynilDescriptor->connect_port = connectPortVynil;
		vynilDescriptor->deactivate = NULL;
		vynilDescriptor->instantiate = instantiateVynil;
		vynilDescriptor->run = runVynil;
		vynilDescriptor->run_adding = runAddingVynil;
		vynilDescriptor->set_run_adding_gain = setRunAddingGainVynil;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (vynilDescriptor) {
		free((LADSPA_PortDescriptor *)vynilDescriptor->PortDescriptors);
		free((char **)vynilDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)vynilDescriptor->PortRangeHints);
		free(vynilDescriptor);
	}

}
