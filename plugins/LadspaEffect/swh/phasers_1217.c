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

#line 10 "phasers_1217.xml"

#include "ladspa-util.h"

#define LFO_SIZE 4096

typedef struct {
  float a1;
  float zm1;
} allpass;

inline static float ap_run(allpass *a, float x)
{
  float y = x * -(a->a1) + a->zm1;
  a->zm1 = y * a->a1 + x;

  return y;
}

inline static void ap_set_delay(allpass *a, float d)
{
  a->a1 = (1.0f - d) / (1.0f + d);
}

inline static void ap_clear(allpass *a)
{
  a->a1  = 0.0f;
  a->zm1 = 0.0f;
}

typedef struct {
  float ga;
  float gr;
  float env;
} envelope;

inline static float env_run(envelope *e, float in)
{
  float env_lvl = e->env;

  in = fabs(in);

  if (env_lvl < in) {
    env_lvl = e->ga * (env_lvl - in) + in;
  } else {
    env_lvl = e->gr * (env_lvl - in) + in;
  }

  e->env = env_lvl;
  return env_lvl;
}

// Set attack time in samples
inline static void env_set_attack(envelope *e, float a)
{
  e->ga = f_exp(-1.0f/a);
}

// Set release time in samples
inline static void env_set_release(envelope *e, float r)
{
  e->gr = f_exp(-1.0f/r);
}

#define LFOPHASER_LFO_RATE             0
#define LFOPHASER_LFO_DEPTH            1
#define LFOPHASER_FB                   2
#define LFOPHASER_SPREAD               3
#define LFOPHASER_INPUT                4
#define LFOPHASER_OUTPUT               5
#define FOURBYFOURPOLE_F0              0
#define FOURBYFOURPOLE_FB0             1
#define FOURBYFOURPOLE_F1              2
#define FOURBYFOURPOLE_FB1             3
#define FOURBYFOURPOLE_F2              4
#define FOURBYFOURPOLE_FB2             5
#define FOURBYFOURPOLE_F3              6
#define FOURBYFOURPOLE_FB3             7
#define FOURBYFOURPOLE_INPUT           8
#define FOURBYFOURPOLE_OUTPUT          9
#define AUTOPHASER_ATTACK_P            0
#define AUTOPHASER_DECAY_P             1
#define AUTOPHASER_DEPTH_P             2
#define AUTOPHASER_FB                  3
#define AUTOPHASER_SPREAD              4
#define AUTOPHASER_INPUT               5
#define AUTOPHASER_OUTPUT              6

static LADSPA_Descriptor *lfoPhaserDescriptor = NULL;

typedef struct {
	LADSPA_Data *lfo_rate;
	LADSPA_Data *lfo_depth;
	LADSPA_Data *fb;
	LADSPA_Data *spread;
	LADSPA_Data *input;
	LADSPA_Data *output;
	allpass *    ap;
	int          count;
	float        f_per_lv;
	int          lfo_pos;
	float *      lfo_tbl;
	float        ym1;
	LADSPA_Data run_adding_gain;
} LfoPhaser;

static LADSPA_Descriptor *fourByFourPoleDescriptor = NULL;

typedef struct {
	LADSPA_Data *f0;
	LADSPA_Data *fb0;
	LADSPA_Data *f1;
	LADSPA_Data *fb1;
	LADSPA_Data *f2;
	LADSPA_Data *fb2;
	LADSPA_Data *f3;
	LADSPA_Data *fb3;
	LADSPA_Data *input;
	LADSPA_Data *output;
	allpass *    ap;
	float        sr_r_2;
	float        y0;
	float        y1;
	float        y2;
	float        y3;
	LADSPA_Data run_adding_gain;
} FourByFourPole;

static LADSPA_Descriptor *autoPhaserDescriptor = NULL;

typedef struct {
	LADSPA_Data *attack_p;
	LADSPA_Data *decay_p;
	LADSPA_Data *depth_p;
	LADSPA_Data *fb;
	LADSPA_Data *spread;
	LADSPA_Data *input;
	LADSPA_Data *output;
	allpass *    ap;
	envelope *   env;
	float        sample_rate;
	float        ym1;
	LADSPA_Data run_adding_gain;
} AutoPhaser;

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
		return lfoPhaserDescriptor;
	case 1:
		return fourByFourPoleDescriptor;
	case 2:
		return autoPhaserDescriptor;
	default:
		return NULL;
	}
}

static void activateLfoPhaser(LADSPA_Handle instance) {
	LfoPhaser *plugin_data = (LfoPhaser *)instance;
	allpass *ap = plugin_data->ap;
	int count = plugin_data->count;
	float f_per_lv = plugin_data->f_per_lv;
	int lfo_pos = plugin_data->lfo_pos;
	float *lfo_tbl = plugin_data->lfo_tbl;
	float ym1 = plugin_data->ym1;
#line 100 "phasers_1217.xml"
	ap_clear(ap);
	ap_clear(ap+1);
	ap_clear(ap+2);
	ap_clear(ap+3);
	ap_clear(ap+4);
	ap_clear(ap+5);
	plugin_data->ap = ap;
	plugin_data->count = count;
	plugin_data->f_per_lv = f_per_lv;
	plugin_data->lfo_pos = lfo_pos;
	plugin_data->lfo_tbl = lfo_tbl;
	plugin_data->ym1 = ym1;

}

static void cleanupLfoPhaser(LADSPA_Handle instance) {
#line 109 "phasers_1217.xml"
	LfoPhaser *plugin_data = (LfoPhaser *)instance;
	free(plugin_data->ap);
	free(plugin_data->lfo_tbl);
	free(instance);
}

static void connectPortLfoPhaser(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	LfoPhaser *plugin;

	plugin = (LfoPhaser *)instance;
	switch (port) {
	case LFOPHASER_LFO_RATE:
		plugin->lfo_rate = data;
		break;
	case LFOPHASER_LFO_DEPTH:
		plugin->lfo_depth = data;
		break;
	case LFOPHASER_FB:
		plugin->fb = data;
		break;
	case LFOPHASER_SPREAD:
		plugin->spread = data;
		break;
	case LFOPHASER_INPUT:
		plugin->input = data;
		break;
	case LFOPHASER_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateLfoPhaser(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	LfoPhaser *plugin_data = (LfoPhaser *)malloc(sizeof(LfoPhaser));
	allpass *ap = NULL;
	int count;
	float f_per_lv;
	int lfo_pos;
	float *lfo_tbl = NULL;
	float ym1;

#line 80 "phasers_1217.xml"
	unsigned int i;
	float p;

	ap = calloc(6, sizeof(allpass));
	ym1 = 0.0f;
	lfo_tbl = malloc(sizeof(float) * LFO_SIZE);
	p = 0.0f;
	for (i=0; i<LFO_SIZE; i++) {
	  p += M_PI * 0.0004882812f;
	  lfo_tbl[i] = (sin(p) + 1.1f) * 0.25f;
	}
	lfo_pos = 0;

	// Frames per lfo value
	f_per_lv = (float)s_rate * 0.0002441406f;

	count = 0;

	plugin_data->ap = ap;
	plugin_data->count = count;
	plugin_data->f_per_lv = f_per_lv;
	plugin_data->lfo_pos = lfo_pos;
	plugin_data->lfo_tbl = lfo_tbl;
	plugin_data->ym1 = ym1;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runLfoPhaser(LADSPA_Handle instance, unsigned long sample_count) {
	LfoPhaser *plugin_data = (LfoPhaser *)instance;

	/* LFO rate (Hz) (float value) */
	const LADSPA_Data lfo_rate = *(plugin_data->lfo_rate);

	/* LFO depth (float value) */
	const LADSPA_Data lfo_depth = *(plugin_data->lfo_depth);

	/* Feedback (float value) */
	const LADSPA_Data fb = *(plugin_data->fb);

	/* Spread (octaves) (float value) */
	const LADSPA_Data spread = *(plugin_data->spread);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	allpass * ap = plugin_data->ap;
	int count = plugin_data->count;
	float f_per_lv = plugin_data->f_per_lv;
	int lfo_pos = plugin_data->lfo_pos;
	float * lfo_tbl = plugin_data->lfo_tbl;
	float ym1 = plugin_data->ym1;

#line 114 "phasers_1217.xml"
	unsigned long pos;
	unsigned int mod;
	float y, d, ofs;

	mod = f_round(f_per_lv / lfo_rate);
	if (mod < 1) {
	  mod=1;
	}

	d = lfo_tbl[lfo_pos];

	for (pos = 0; pos < sample_count; pos++) {
	  // Get new value for LFO if needed
	  if (++count % mod == 0) {
	    lfo_pos++;
	    lfo_pos &= 0x7FF;
	    count = 0;
	    d = lfo_tbl[lfo_pos] * lfo_depth;

	    ap_set_delay(ap, d);
	    ofs = spread * 0.01562f;
	    ap_set_delay(ap+1, d+ofs);
	    ofs *= 2.0f;
	    ap_set_delay(ap+2, d+ofs);
	    ofs *= 2.0f;
	    ap_set_delay(ap+3, d+ofs);
	    ofs *= 2.0f;
	    ap_set_delay(ap+4, d+ofs);
	    ofs *= 2.0f;
	    ap_set_delay(ap+5, d+ofs);

	  }
	  //Run in series, doesn't quite sound as nice
	  y = ap_run(ap, input[pos] + ym1 * fb);
	  y = ap_run(ap+1, y);
	  y = ap_run(ap+2, y);
	  y = ap_run(ap+3, y);
	  y = ap_run(ap+4, y);
	  y = ap_run(ap+5, y);

	  buffer_write(output[pos], y);
	  ym1 = y;
	}

	plugin_data->ym1 = ym1;
	plugin_data->count = count;
	plugin_data->lfo_pos = lfo_pos;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainLfoPhaser(LADSPA_Handle instance, LADSPA_Data gain) {
	((LfoPhaser *)instance)->run_adding_gain = gain;
}

static void runAddingLfoPhaser(LADSPA_Handle instance, unsigned long sample_count) {
	LfoPhaser *plugin_data = (LfoPhaser *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* LFO rate (Hz) (float value) */
	const LADSPA_Data lfo_rate = *(plugin_data->lfo_rate);

	/* LFO depth (float value) */
	const LADSPA_Data lfo_depth = *(plugin_data->lfo_depth);

	/* Feedback (float value) */
	const LADSPA_Data fb = *(plugin_data->fb);

	/* Spread (octaves) (float value) */
	const LADSPA_Data spread = *(plugin_data->spread);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	allpass * ap = plugin_data->ap;
	int count = plugin_data->count;
	float f_per_lv = plugin_data->f_per_lv;
	int lfo_pos = plugin_data->lfo_pos;
	float * lfo_tbl = plugin_data->lfo_tbl;
	float ym1 = plugin_data->ym1;

#line 114 "phasers_1217.xml"
	unsigned long pos;
	unsigned int mod;
	float y, d, ofs;

	mod = f_round(f_per_lv / lfo_rate);
	if (mod < 1) {
	  mod=1;
	}

	d = lfo_tbl[lfo_pos];

	for (pos = 0; pos < sample_count; pos++) {
	  // Get new value for LFO if needed
	  if (++count % mod == 0) {
	    lfo_pos++;
	    lfo_pos &= 0x7FF;
	    count = 0;
	    d = lfo_tbl[lfo_pos] * lfo_depth;

	    ap_set_delay(ap, d);
	    ofs = spread * 0.01562f;
	    ap_set_delay(ap+1, d+ofs);
	    ofs *= 2.0f;
	    ap_set_delay(ap+2, d+ofs);
	    ofs *= 2.0f;
	    ap_set_delay(ap+3, d+ofs);
	    ofs *= 2.0f;
	    ap_set_delay(ap+4, d+ofs);
	    ofs *= 2.0f;
	    ap_set_delay(ap+5, d+ofs);

	  }
	  //Run in series, doesn't quite sound as nice
	  y = ap_run(ap, input[pos] + ym1 * fb);
	  y = ap_run(ap+1, y);
	  y = ap_run(ap+2, y);
	  y = ap_run(ap+3, y);
	  y = ap_run(ap+4, y);
	  y = ap_run(ap+5, y);

	  buffer_write(output[pos], y);
	  ym1 = y;
	}

	plugin_data->ym1 = ym1;
	plugin_data->count = count;
	plugin_data->lfo_pos = lfo_pos;
}

static void activateFourByFourPole(LADSPA_Handle instance) {
	FourByFourPole *plugin_data = (FourByFourPole *)instance;
	allpass *ap = plugin_data->ap;
	float sr_r_2 = plugin_data->sr_r_2;
	float y0 = plugin_data->y0;
	float y1 = plugin_data->y1;
	float y2 = plugin_data->y2;
	float y3 = plugin_data->y3;
#line 100 "phasers_1217.xml"
	ap_clear(ap);
	ap_clear(ap+1);
	ap_clear(ap+2);
	ap_clear(ap+3);
	ap_clear(ap+4);
	ap_clear(ap+5);
	ap_clear(ap+6);
	ap_clear(ap+7);
	ap_clear(ap+8);
	ap_clear(ap+9);
	ap_clear(ap+10);
	ap_clear(ap+11);
	ap_clear(ap+12);
	ap_clear(ap+13);
	ap_clear(ap+14);
	ap_clear(ap+15);
	plugin_data->ap = ap;
	plugin_data->sr_r_2 = sr_r_2;
	plugin_data->y0 = y0;
	plugin_data->y1 = y1;
	plugin_data->y2 = y2;
	plugin_data->y3 = y3;

}

static void cleanupFourByFourPole(LADSPA_Handle instance) {
#line 109 "phasers_1217.xml"
	FourByFourPole *plugin_data = (FourByFourPole *)instance;
	free(plugin_data->ap);
	free(instance);
}

static void connectPortFourByFourPole(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	FourByFourPole *plugin;

	plugin = (FourByFourPole *)instance;
	switch (port) {
	case FOURBYFOURPOLE_F0:
		plugin->f0 = data;
		break;
	case FOURBYFOURPOLE_FB0:
		plugin->fb0 = data;
		break;
	case FOURBYFOURPOLE_F1:
		plugin->f1 = data;
		break;
	case FOURBYFOURPOLE_FB1:
		plugin->fb1 = data;
		break;
	case FOURBYFOURPOLE_F2:
		plugin->f2 = data;
		break;
	case FOURBYFOURPOLE_FB2:
		plugin->fb2 = data;
		break;
	case FOURBYFOURPOLE_F3:
		plugin->f3 = data;
		break;
	case FOURBYFOURPOLE_FB3:
		plugin->fb3 = data;
		break;
	case FOURBYFOURPOLE_INPUT:
		plugin->input = data;
		break;
	case FOURBYFOURPOLE_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateFourByFourPole(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	FourByFourPole *plugin_data = (FourByFourPole *)malloc(sizeof(FourByFourPole));
	allpass *ap = NULL;
	float sr_r_2;
	float y0;
	float y1;
	float y2;
	float y3;

#line 80 "phasers_1217.xml"
	ap = calloc(16, sizeof(allpass));
	y0 = 0.0f;
	y1 = 0.0f;
	y2 = 0.0f;
	y3 = 0.0f;
	sr_r_2 = 1.0f / s_rate;

	plugin_data->ap = ap;
	plugin_data->sr_r_2 = sr_r_2;
	plugin_data->y0 = y0;
	plugin_data->y1 = y1;
	plugin_data->y2 = y2;
	plugin_data->y3 = y3;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runFourByFourPole(LADSPA_Handle instance, unsigned long sample_count) {
	FourByFourPole *plugin_data = (FourByFourPole *)instance;

	/* Frequency 1 (float value) */
	const LADSPA_Data f0 = *(plugin_data->f0);

	/* Feedback 1 (float value) */
	const LADSPA_Data fb0 = *(plugin_data->fb0);

	/* Frequency 2 (float value) */
	const LADSPA_Data f1 = *(plugin_data->f1);

	/* Feedback 2 (float value) */
	const LADSPA_Data fb1 = *(plugin_data->fb1);

	/* Frequency 3 (float value) */
	const LADSPA_Data f2 = *(plugin_data->f2);

	/* Feedback 3 (float value) */
	const LADSPA_Data fb2 = *(plugin_data->fb2);

	/* Frequency 4 (float value) */
	const LADSPA_Data f3 = *(plugin_data->f3);

	/* Feedback 4 (float value) */
	const LADSPA_Data fb3 = *(plugin_data->fb3);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	allpass * ap = plugin_data->ap;
	float sr_r_2 = plugin_data->sr_r_2;
	float y0 = plugin_data->y0;
	float y1 = plugin_data->y1;
	float y2 = plugin_data->y2;
	float y3 = plugin_data->y3;

#line 114 "phasers_1217.xml"
	unsigned long pos;

	ap_set_delay(ap,   f0 * sr_r_2);
	ap_set_delay(ap+1, f0 * sr_r_2);
	ap_set_delay(ap+2, f0 * sr_r_2);
	ap_set_delay(ap+3, f0 * sr_r_2);
	ap_set_delay(ap+4, f1 * sr_r_2);
	ap_set_delay(ap+5, f1 * sr_r_2);
	ap_set_delay(ap+6, f1 * sr_r_2);
	ap_set_delay(ap+7, f1 * sr_r_2);
	ap_set_delay(ap+8, f2 * sr_r_2);
	ap_set_delay(ap+9, f2 * sr_r_2);
	ap_set_delay(ap+10, f2 * sr_r_2);
	ap_set_delay(ap+11, f2 * sr_r_2);
	ap_set_delay(ap+12, f3 * sr_r_2);
	ap_set_delay(ap+13, f3 * sr_r_2);
	ap_set_delay(ap+14, f3 * sr_r_2);
	ap_set_delay(ap+15, f3 * sr_r_2);

	for (pos = 0; pos < sample_count; pos++) {
	  y0 = ap_run(ap,   input[pos] + y0 * fb0);
	  y0 = ap_run(ap+1,   y0);
	  y0 = ap_run(ap+2,   y0);
	  y0 = ap_run(ap+3,   y0);

	  y1 = ap_run(ap+4,   y0 + y1 * fb1);
	  y1 = ap_run(ap+5,   y1);
	  y1 = ap_run(ap+6,   y1);
	  y1 = ap_run(ap+7,   y1);

	  y2 = ap_run(ap+8,  y1 + y2 * fb2);
	  y2 = ap_run(ap+9,  y2);
	  y2 = ap_run(ap+10, y2);
	  y2 = ap_run(ap+11, y2);

	  y3 = ap_run(ap+12, y2 + y3 * fb3);
	  y3 = ap_run(ap+13, y3);
	  y3 = ap_run(ap+14, y3);
	  y3 = ap_run(ap+15, y3);

	  buffer_write(output[pos], y3);
	}

	plugin_data->y0 = y0;
	plugin_data->y1 = y1;
	plugin_data->y2 = y2;
	plugin_data->y3 = y3;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainFourByFourPole(LADSPA_Handle instance, LADSPA_Data gain) {
	((FourByFourPole *)instance)->run_adding_gain = gain;
}

static void runAddingFourByFourPole(LADSPA_Handle instance, unsigned long sample_count) {
	FourByFourPole *plugin_data = (FourByFourPole *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Frequency 1 (float value) */
	const LADSPA_Data f0 = *(plugin_data->f0);

	/* Feedback 1 (float value) */
	const LADSPA_Data fb0 = *(plugin_data->fb0);

	/* Frequency 2 (float value) */
	const LADSPA_Data f1 = *(plugin_data->f1);

	/* Feedback 2 (float value) */
	const LADSPA_Data fb1 = *(plugin_data->fb1);

	/* Frequency 3 (float value) */
	const LADSPA_Data f2 = *(plugin_data->f2);

	/* Feedback 3 (float value) */
	const LADSPA_Data fb2 = *(plugin_data->fb2);

	/* Frequency 4 (float value) */
	const LADSPA_Data f3 = *(plugin_data->f3);

	/* Feedback 4 (float value) */
	const LADSPA_Data fb3 = *(plugin_data->fb3);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	allpass * ap = plugin_data->ap;
	float sr_r_2 = plugin_data->sr_r_2;
	float y0 = plugin_data->y0;
	float y1 = plugin_data->y1;
	float y2 = plugin_data->y2;
	float y3 = plugin_data->y3;

#line 114 "phasers_1217.xml"
	unsigned long pos;

	ap_set_delay(ap,   f0 * sr_r_2);
	ap_set_delay(ap+1, f0 * sr_r_2);
	ap_set_delay(ap+2, f0 * sr_r_2);
	ap_set_delay(ap+3, f0 * sr_r_2);
	ap_set_delay(ap+4, f1 * sr_r_2);
	ap_set_delay(ap+5, f1 * sr_r_2);
	ap_set_delay(ap+6, f1 * sr_r_2);
	ap_set_delay(ap+7, f1 * sr_r_2);
	ap_set_delay(ap+8, f2 * sr_r_2);
	ap_set_delay(ap+9, f2 * sr_r_2);
	ap_set_delay(ap+10, f2 * sr_r_2);
	ap_set_delay(ap+11, f2 * sr_r_2);
	ap_set_delay(ap+12, f3 * sr_r_2);
	ap_set_delay(ap+13, f3 * sr_r_2);
	ap_set_delay(ap+14, f3 * sr_r_2);
	ap_set_delay(ap+15, f3 * sr_r_2);

	for (pos = 0; pos < sample_count; pos++) {
	  y0 = ap_run(ap,   input[pos] + y0 * fb0);
	  y0 = ap_run(ap+1,   y0);
	  y0 = ap_run(ap+2,   y0);
	  y0 = ap_run(ap+3,   y0);

	  y1 = ap_run(ap+4,   y0 + y1 * fb1);
	  y1 = ap_run(ap+5,   y1);
	  y1 = ap_run(ap+6,   y1);
	  y1 = ap_run(ap+7,   y1);

	  y2 = ap_run(ap+8,  y1 + y2 * fb2);
	  y2 = ap_run(ap+9,  y2);
	  y2 = ap_run(ap+10, y2);
	  y2 = ap_run(ap+11, y2);

	  y3 = ap_run(ap+12, y2 + y3 * fb3);
	  y3 = ap_run(ap+13, y3);
	  y3 = ap_run(ap+14, y3);
	  y3 = ap_run(ap+15, y3);

	  buffer_write(output[pos], y3);
	}

	plugin_data->y0 = y0;
	plugin_data->y1 = y1;
	plugin_data->y2 = y2;
	plugin_data->y3 = y3;
}

static void activateAutoPhaser(LADSPA_Handle instance) {
	AutoPhaser *plugin_data = (AutoPhaser *)instance;
	allpass *ap = plugin_data->ap;
	envelope *env = plugin_data->env;
	float sample_rate = plugin_data->sample_rate;
	float ym1 = plugin_data->ym1;
#line 100 "phasers_1217.xml"
	ap_clear(ap);
	ap_clear(ap+1);
	ap_clear(ap+2);
	ap_clear(ap+3);
	ap_clear(ap+4);
	ap_clear(ap+5);
	plugin_data->ap = ap;
	plugin_data->env = env;
	plugin_data->sample_rate = sample_rate;
	plugin_data->ym1 = ym1;

}

static void cleanupAutoPhaser(LADSPA_Handle instance) {
#line 109 "phasers_1217.xml"
	AutoPhaser *plugin_data = (AutoPhaser *)instance;
	free(plugin_data->ap);
	free(plugin_data->env);
	free(instance);
}

static void connectPortAutoPhaser(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	AutoPhaser *plugin;

	plugin = (AutoPhaser *)instance;
	switch (port) {
	case AUTOPHASER_ATTACK_P:
		plugin->attack_p = data;
		break;
	case AUTOPHASER_DECAY_P:
		plugin->decay_p = data;
		break;
	case AUTOPHASER_DEPTH_P:
		plugin->depth_p = data;
		break;
	case AUTOPHASER_FB:
		plugin->fb = data;
		break;
	case AUTOPHASER_SPREAD:
		plugin->spread = data;
		break;
	case AUTOPHASER_INPUT:
		plugin->input = data;
		break;
	case AUTOPHASER_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateAutoPhaser(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	AutoPhaser *plugin_data = (AutoPhaser *)malloc(sizeof(AutoPhaser));
	allpass *ap = NULL;
	envelope *env = NULL;
	float sample_rate;
	float ym1;

#line 80 "phasers_1217.xml"
	ap = calloc(6, sizeof(allpass));
	env = calloc(1, sizeof(envelope));
	ym1 = 0.0f;
	sample_rate = (float)s_rate;

	plugin_data->ap = ap;
	plugin_data->env = env;
	plugin_data->sample_rate = sample_rate;
	plugin_data->ym1 = ym1;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runAutoPhaser(LADSPA_Handle instance, unsigned long sample_count) {
	AutoPhaser *plugin_data = (AutoPhaser *)instance;

	/* Attack time (s) (float value) */
	const LADSPA_Data attack_p = *(plugin_data->attack_p);

	/* Decay time (s) (float value) */
	const LADSPA_Data decay_p = *(plugin_data->decay_p);

	/* Modulation depth (float value) */
	const LADSPA_Data depth_p = *(plugin_data->depth_p);

	/* Feedback (float value) */
	const LADSPA_Data fb = *(plugin_data->fb);

	/* Spread (octaves) (float value) */
	const LADSPA_Data spread = *(plugin_data->spread);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	allpass * ap = plugin_data->ap;
	envelope * env = plugin_data->env;
	float sample_rate = plugin_data->sample_rate;
	float ym1 = plugin_data->ym1;

#line 114 "phasers_1217.xml"
	unsigned long pos;
	float y, d, ofs;
	float attack = attack_p;
	float decay = decay_p;
	const float depth = depth_p * 0.5f;

	if (attack < 0.01f) {
	  attack = 0.01f;
	}
	if (decay < 0.01f) {
	  decay = 0.01f;
	}
	env_set_attack(env, attack * sample_rate * 0.25f);
	env_set_release(env, decay * sample_rate * 0.25f);


	for (pos = 0; pos < sample_count; pos++) {
	  if (pos % 4 == 0) {
	    d = env_run(env, input[pos]) * depth;
	    ap_set_delay(ap, d);
	    ofs = spread * 0.01562f;
	    ap_set_delay(ap+1, d+ofs);
	    ofs *= 2.0f;
	    ap_set_delay(ap+2, d+ofs);
	    ofs *= 2.0f;
	    ap_set_delay(ap+3, d+ofs);
	    ofs *= 2.0f;
	    ap_set_delay(ap+4, d+ofs);
	    ofs *= 2.0f;
	    ap_set_delay(ap+5, d+ofs);
	  }

	  //Run allpass filters in series
	  y = ap_run(ap, input[pos] + ym1 * fb);
	  y = ap_run(ap+1, y);
	  y = ap_run(ap+2, y);
	  y = ap_run(ap+3, y);
	  y = ap_run(ap+4, y);
	  y = ap_run(ap+5, y);

	  buffer_write(output[pos], y);
	  ym1 = y;
	}

	plugin_data->ym1 = ym1;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainAutoPhaser(LADSPA_Handle instance, LADSPA_Data gain) {
	((AutoPhaser *)instance)->run_adding_gain = gain;
}

static void runAddingAutoPhaser(LADSPA_Handle instance, unsigned long sample_count) {
	AutoPhaser *plugin_data = (AutoPhaser *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Attack time (s) (float value) */
	const LADSPA_Data attack_p = *(plugin_data->attack_p);

	/* Decay time (s) (float value) */
	const LADSPA_Data decay_p = *(plugin_data->decay_p);

	/* Modulation depth (float value) */
	const LADSPA_Data depth_p = *(plugin_data->depth_p);

	/* Feedback (float value) */
	const LADSPA_Data fb = *(plugin_data->fb);

	/* Spread (octaves) (float value) */
	const LADSPA_Data spread = *(plugin_data->spread);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	allpass * ap = plugin_data->ap;
	envelope * env = plugin_data->env;
	float sample_rate = plugin_data->sample_rate;
	float ym1 = plugin_data->ym1;

#line 114 "phasers_1217.xml"
	unsigned long pos;
	float y, d, ofs;
	float attack = attack_p;
	float decay = decay_p;
	const float depth = depth_p * 0.5f;

	if (attack < 0.01f) {
	  attack = 0.01f;
	}
	if (decay < 0.01f) {
	  decay = 0.01f;
	}
	env_set_attack(env, attack * sample_rate * 0.25f);
	env_set_release(env, decay * sample_rate * 0.25f);


	for (pos = 0; pos < sample_count; pos++) {
	  if (pos % 4 == 0) {
	    d = env_run(env, input[pos]) * depth;
	    ap_set_delay(ap, d);
	    ofs = spread * 0.01562f;
	    ap_set_delay(ap+1, d+ofs);
	    ofs *= 2.0f;
	    ap_set_delay(ap+2, d+ofs);
	    ofs *= 2.0f;
	    ap_set_delay(ap+3, d+ofs);
	    ofs *= 2.0f;
	    ap_set_delay(ap+4, d+ofs);
	    ofs *= 2.0f;
	    ap_set_delay(ap+5, d+ofs);
	  }

	  //Run allpass filters in series
	  y = ap_run(ap, input[pos] + ym1 * fb);
	  y = ap_run(ap+1, y);
	  y = ap_run(ap+2, y);
	  y = ap_run(ap+3, y);
	  y = ap_run(ap+4, y);
	  y = ap_run(ap+5, y);

	  buffer_write(output[pos], y);
	  ym1 = y;
	}

	plugin_data->ym1 = ym1;
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


	lfoPhaserDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (lfoPhaserDescriptor) {
		lfoPhaserDescriptor->UniqueID = 1217;
		lfoPhaserDescriptor->Label = "lfoPhaser";
		lfoPhaserDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		lfoPhaserDescriptor->Name =
		 D_("LFO Phaser");
		lfoPhaserDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		lfoPhaserDescriptor->Copyright =
		 "GPL";
		lfoPhaserDescriptor->PortCount = 6;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(6,
		 sizeof(LADSPA_PortDescriptor));
		lfoPhaserDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(6,
		 sizeof(LADSPA_PortRangeHint));
		lfoPhaserDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(6, sizeof(char*));
		lfoPhaserDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for LFO rate (Hz) */
		port_descriptors[LFOPHASER_LFO_RATE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LFOPHASER_LFO_RATE] =
		 D_("LFO rate (Hz)");
		port_range_hints[LFOPHASER_LFO_RATE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[LFOPHASER_LFO_RATE].LowerBound = 0;
		port_range_hints[LFOPHASER_LFO_RATE].UpperBound = 100;

		/* Parameters for LFO depth */
		port_descriptors[LFOPHASER_LFO_DEPTH] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LFOPHASER_LFO_DEPTH] =
		 D_("LFO depth");
		port_range_hints[LFOPHASER_LFO_DEPTH].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[LFOPHASER_LFO_DEPTH].LowerBound = 0;
		port_range_hints[LFOPHASER_LFO_DEPTH].UpperBound = 1;

		/* Parameters for Feedback */
		port_descriptors[LFOPHASER_FB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LFOPHASER_FB] =
		 D_("Feedback");
		port_range_hints[LFOPHASER_FB].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[LFOPHASER_FB].LowerBound = -1;
		port_range_hints[LFOPHASER_FB].UpperBound = 1;

		/* Parameters for Spread (octaves) */
		port_descriptors[LFOPHASER_SPREAD] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LFOPHASER_SPREAD] =
		 D_("Spread (octaves)");
		port_range_hints[LFOPHASER_SPREAD].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[LFOPHASER_SPREAD].LowerBound = 0;
		port_range_hints[LFOPHASER_SPREAD].UpperBound = 2;

		/* Parameters for Input */
		port_descriptors[LFOPHASER_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[LFOPHASER_INPUT] =
		 D_("Input");
		port_range_hints[LFOPHASER_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[LFOPHASER_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[LFOPHASER_OUTPUT] =
		 D_("Output");
		port_range_hints[LFOPHASER_OUTPUT].HintDescriptor = 0;

		lfoPhaserDescriptor->activate = activateLfoPhaser;
		lfoPhaserDescriptor->cleanup = cleanupLfoPhaser;
		lfoPhaserDescriptor->connect_port = connectPortLfoPhaser;
		lfoPhaserDescriptor->deactivate = NULL;
		lfoPhaserDescriptor->instantiate = instantiateLfoPhaser;
		lfoPhaserDescriptor->run = runLfoPhaser;
		lfoPhaserDescriptor->run_adding = runAddingLfoPhaser;
		lfoPhaserDescriptor->set_run_adding_gain = setRunAddingGainLfoPhaser;
	}

	fourByFourPoleDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (fourByFourPoleDescriptor) {
		fourByFourPoleDescriptor->UniqueID = 1218;
		fourByFourPoleDescriptor->Label = "fourByFourPole";
		fourByFourPoleDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		fourByFourPoleDescriptor->Name =
		 D_("4 x 4 pole allpass");
		fourByFourPoleDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		fourByFourPoleDescriptor->Copyright =
		 "GPL";
		fourByFourPoleDescriptor->PortCount = 10;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(10,
		 sizeof(LADSPA_PortDescriptor));
		fourByFourPoleDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(10,
		 sizeof(LADSPA_PortRangeHint));
		fourByFourPoleDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(10, sizeof(char*));
		fourByFourPoleDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Frequency 1 */
		port_descriptors[FOURBYFOURPOLE_F0] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[FOURBYFOURPOLE_F0] =
		 D_("Frequency 1");
		port_range_hints[FOURBYFOURPOLE_F0].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[FOURBYFOURPOLE_F0].LowerBound = 1;
		port_range_hints[FOURBYFOURPOLE_F0].UpperBound = 20000;

		/* Parameters for Feedback 1 */
		port_descriptors[FOURBYFOURPOLE_FB0] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[FOURBYFOURPOLE_FB0] =
		 D_("Feedback 1");
		port_range_hints[FOURBYFOURPOLE_FB0].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[FOURBYFOURPOLE_FB0].LowerBound = -1;
		port_range_hints[FOURBYFOURPOLE_FB0].UpperBound = 1;

		/* Parameters for Frequency 2 */
		port_descriptors[FOURBYFOURPOLE_F1] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[FOURBYFOURPOLE_F1] =
		 D_("Frequency 2");
		port_range_hints[FOURBYFOURPOLE_F1].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[FOURBYFOURPOLE_F1].LowerBound = 1;
		port_range_hints[FOURBYFOURPOLE_F1].UpperBound = 20000;

		/* Parameters for Feedback 2 */
		port_descriptors[FOURBYFOURPOLE_FB1] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[FOURBYFOURPOLE_FB1] =
		 D_("Feedback 2");
		port_range_hints[FOURBYFOURPOLE_FB1].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[FOURBYFOURPOLE_FB1].LowerBound = -1;
		port_range_hints[FOURBYFOURPOLE_FB1].UpperBound = 1;

		/* Parameters for Frequency 3 */
		port_descriptors[FOURBYFOURPOLE_F2] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[FOURBYFOURPOLE_F2] =
		 D_("Frequency 3");
		port_range_hints[FOURBYFOURPOLE_F2].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_HIGH;
		port_range_hints[FOURBYFOURPOLE_F2].LowerBound = 1;
		port_range_hints[FOURBYFOURPOLE_F2].UpperBound = 20000;

		/* Parameters for Feedback 3 */
		port_descriptors[FOURBYFOURPOLE_FB2] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[FOURBYFOURPOLE_FB2] =
		 D_("Feedback 3");
		port_range_hints[FOURBYFOURPOLE_FB2].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[FOURBYFOURPOLE_FB2].LowerBound = -1;
		port_range_hints[FOURBYFOURPOLE_FB2].UpperBound = 1;

		/* Parameters for Frequency 4 */
		port_descriptors[FOURBYFOURPOLE_F3] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[FOURBYFOURPOLE_F3] =
		 D_("Frequency 4");
		port_range_hints[FOURBYFOURPOLE_F3].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MAXIMUM;
		port_range_hints[FOURBYFOURPOLE_F3].LowerBound = 1;
		port_range_hints[FOURBYFOURPOLE_F3].UpperBound = 20000;

		/* Parameters for Feedback 4 */
		port_descriptors[FOURBYFOURPOLE_FB3] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[FOURBYFOURPOLE_FB3] =
		 D_("Feedback 4");
		port_range_hints[FOURBYFOURPOLE_FB3].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[FOURBYFOURPOLE_FB3].LowerBound = -1;
		port_range_hints[FOURBYFOURPOLE_FB3].UpperBound = 1;

		/* Parameters for Input */
		port_descriptors[FOURBYFOURPOLE_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[FOURBYFOURPOLE_INPUT] =
		 D_("Input");
		port_range_hints[FOURBYFOURPOLE_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[FOURBYFOURPOLE_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[FOURBYFOURPOLE_OUTPUT] =
		 D_("Output");
		port_range_hints[FOURBYFOURPOLE_OUTPUT].HintDescriptor = 0;

		fourByFourPoleDescriptor->activate = activateFourByFourPole;
		fourByFourPoleDescriptor->cleanup = cleanupFourByFourPole;
		fourByFourPoleDescriptor->connect_port = connectPortFourByFourPole;
		fourByFourPoleDescriptor->deactivate = NULL;
		fourByFourPoleDescriptor->instantiate = instantiateFourByFourPole;
		fourByFourPoleDescriptor->run = runFourByFourPole;
		fourByFourPoleDescriptor->run_adding = runAddingFourByFourPole;
		fourByFourPoleDescriptor->set_run_adding_gain = setRunAddingGainFourByFourPole;
	}

	autoPhaserDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (autoPhaserDescriptor) {
		autoPhaserDescriptor->UniqueID = 1219;
		autoPhaserDescriptor->Label = "autoPhaser";
		autoPhaserDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		autoPhaserDescriptor->Name =
		 D_("Auto phaser");
		autoPhaserDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		autoPhaserDescriptor->Copyright =
		 "GPL";
		autoPhaserDescriptor->PortCount = 7;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(7,
		 sizeof(LADSPA_PortDescriptor));
		autoPhaserDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(7,
		 sizeof(LADSPA_PortRangeHint));
		autoPhaserDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(7, sizeof(char*));
		autoPhaserDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Attack time (s) */
		port_descriptors[AUTOPHASER_ATTACK_P] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[AUTOPHASER_ATTACK_P] =
		 D_("Attack time (s)");
		port_range_hints[AUTOPHASER_ATTACK_P].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[AUTOPHASER_ATTACK_P].LowerBound = 0;
		port_range_hints[AUTOPHASER_ATTACK_P].UpperBound = 1;

		/* Parameters for Decay time (s) */
		port_descriptors[AUTOPHASER_DECAY_P] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[AUTOPHASER_DECAY_P] =
		 D_("Decay time (s)");
		port_range_hints[AUTOPHASER_DECAY_P].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[AUTOPHASER_DECAY_P].LowerBound = 0;
		port_range_hints[AUTOPHASER_DECAY_P].UpperBound = 1;

		/* Parameters for Modulation depth */
		port_descriptors[AUTOPHASER_DEPTH_P] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[AUTOPHASER_DEPTH_P] =
		 D_("Modulation depth");
		port_range_hints[AUTOPHASER_DEPTH_P].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[AUTOPHASER_DEPTH_P].LowerBound = 0;
		port_range_hints[AUTOPHASER_DEPTH_P].UpperBound = 1;

		/* Parameters for Feedback */
		port_descriptors[AUTOPHASER_FB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[AUTOPHASER_FB] =
		 D_("Feedback");
		port_range_hints[AUTOPHASER_FB].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[AUTOPHASER_FB].LowerBound = -1;
		port_range_hints[AUTOPHASER_FB].UpperBound = 1;

		/* Parameters for Spread (octaves) */
		port_descriptors[AUTOPHASER_SPREAD] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[AUTOPHASER_SPREAD] =
		 D_("Spread (octaves)");
		port_range_hints[AUTOPHASER_SPREAD].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[AUTOPHASER_SPREAD].LowerBound = 0;
		port_range_hints[AUTOPHASER_SPREAD].UpperBound = 2;

		/* Parameters for Input */
		port_descriptors[AUTOPHASER_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[AUTOPHASER_INPUT] =
		 D_("Input");
		port_range_hints[AUTOPHASER_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[AUTOPHASER_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[AUTOPHASER_OUTPUT] =
		 D_("Output");
		port_range_hints[AUTOPHASER_OUTPUT].HintDescriptor = 0;

		autoPhaserDescriptor->activate = activateAutoPhaser;
		autoPhaserDescriptor->cleanup = cleanupAutoPhaser;
		autoPhaserDescriptor->connect_port = connectPortAutoPhaser;
		autoPhaserDescriptor->deactivate = NULL;
		autoPhaserDescriptor->instantiate = instantiateAutoPhaser;
		autoPhaserDescriptor->run = runAutoPhaser;
		autoPhaserDescriptor->run_adding = runAddingAutoPhaser;
		autoPhaserDescriptor->set_run_adding_gain = setRunAddingGainAutoPhaser;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (lfoPhaserDescriptor) {
		free((LADSPA_PortDescriptor *)lfoPhaserDescriptor->PortDescriptors);
		free((char **)lfoPhaserDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)lfoPhaserDescriptor->PortRangeHints);
		free(lfoPhaserDescriptor);
	}
	if (fourByFourPoleDescriptor) {
		free((LADSPA_PortDescriptor *)fourByFourPoleDescriptor->PortDescriptors);
		free((char **)fourByFourPoleDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)fourByFourPoleDescriptor->PortRangeHints);
		free(fourByFourPoleDescriptor);
	}
	if (autoPhaserDescriptor) {
		free((LADSPA_PortDescriptor *)autoPhaserDescriptor->PortDescriptors);
		free((char **)autoPhaserDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)autoPhaserDescriptor->PortRangeHints);
		free(autoPhaserDescriptor);
	}

}
