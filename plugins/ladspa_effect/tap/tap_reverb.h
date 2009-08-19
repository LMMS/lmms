/*                                                     -*- linux-c -*-
    Copyright (C) 2004 Tom Szilagyi
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    $Id: tap_reverb.h,v 1.10 2004/06/14 16:43:55 tszilagyi Exp $
*/
#ifndef _ISOC99_SOURCE
#define _ISOC99_SOURCE
#endif

#include <stdint.h>



/* The Unique ID of the plugin: */

#define ID_STEREO       2142

/* The port numbers for the plugin: */

#define DECAY       0
#define DRYLEVEL    1
#define WETLEVEL    2
#define COMBS_EN    3  /* comb filters on/off */
#define ALLPS_EN    4  /* allpass filters on/off */
#define BANDPASS_EN 5  /* bandpass filters on/off */
#define STEREO_ENH  6  /* stereo enhanced mode on/off */
#define MODE        7

#define INPUT_L     8
#define OUTPUT_L    9
#define INPUT_R     10
#define OUTPUT_R    11

/* Total number of ports */

#define PORTCOUNT_STEREO 12

/* Global constants (times in ms, bwidth in octaves) */

#define MAX_COMBS         20
#define MAX_ALLPS         20
#define MAX_DECAY         10000.0f
#define MAX_COMB_DELAY    250.0f
#define MAX_ALLP_DELAY    20.0f
#define BANDPASS_BWIDTH   1.5f
#define FREQ_RESP_BWIDTH  3.0f
#define ENH_STEREO_RATIO  0.998f

/* compensation ratio of freq_resp in fb_gain calc */
#define FR_R_COMP         0.75f


#ifndef M_PI
#define M_PI 3.14159265358979323846264338327
#endif


/* push a sample into a ringbuffer and return the sample falling out */
static inline
rev_t
push_buffer(rev_t insample, rev_t * buffer,
            unsigned long buflen, unsigned long * pos) {

        rev_t outsample;

        outsample = buffer[*pos];
        buffer[(*pos)++] = insample;

        if (*pos >= buflen)
                *pos = 0;

        return outsample;
}

/* read a value from a ringbuffer.
 * n == 0 returns the oldest sample from the buffer.
 * n == buflen-1 returns the sample written to the buffer
 *      at the last push_buffer call.
 * n must not exceed buflen-1, or your computer will explode.
 */
static inline
rev_t
read_buffer(rev_t * buffer, unsigned long buflen,
            unsigned long pos, unsigned long n) {

        while (n + pos >= buflen)
                n -= buflen;
        return buffer[n + pos];
}


/* overwrites a value in a ringbuffer, but pos stays the same.
 * n == 0 overwrites the oldest sample pushed in the buffer.
 * n == buflen-1 overwrites the sample written to the buffer
 *      at the last push_buffer call.
 * n must not exceed buflen-1, or your computer... you know.
 */
static inline
void
write_buffer(rev_t insample, rev_t * buffer, unsigned long buflen,
             unsigned long pos, unsigned long n) {

        while (n + pos >= buflen)
                n -= buflen;
        buffer[n + pos] = insample;
}

#define db2lin(x) ((x) > -90.0f ? powf(10.0f, (x) * 0.05f) : 0.0f)
#define ABS(x)  (x)>0.0f?(x):-1.0f*(x)
#define LN_2_2 0.34657359f
#define LIMIT(v,l,u) ((v)<(l)?(l):((v)>(u)?(u):(v)))

#define BIQUAD_TYPE float
typedef BIQUAD_TYPE bq_t;

typedef struct {
        bq_t a1;
        bq_t a2;
        bq_t b0;
        bq_t b1;
        bq_t b2;
        rev_t x1;
        rev_t x2;
        rev_t y1;
        rev_t y2;
} biquad;


static inline void biquad_init(biquad *f) {

        f->x1 = 0.0f;
        f->x2 = 0.0f;
        f->y1 = 0.0f;
        f->y2 = 0.0f;
}

static inline
void
eq_set_params(biquad *f, bq_t fc, bq_t gain, bq_t bw, bq_t fs) {

        bq_t w = 2.0f * M_PI * LIMIT(fc, 1.0f, fs/2.0f) / fs;
        bq_t cw = cosf(w);
        bq_t sw = sinf(w);
        bq_t J = pow(10.0f, gain * 0.025f);
        bq_t g = sw * sinhf(LN_2_2 * LIMIT(bw, 0.0001f, 4.0f) * w / sw);
        bq_t a0r = 1.0f / (1.0f + (g / J));

        f->b0 = (1.0f + (g * J)) * a0r;
        f->b1 = (-2.0f * cw) * a0r;
        f->b2 = (1.0f - (g * J)) * a0r;
        f->a1 = -(f->b1);
        f->a2 = ((g / J) - 1.0f) * a0r;
}

static inline void lp_set_params(biquad *f, bq_t fc, bq_t bw, bq_t fs) {
        bq_t omega = 2.0 * M_PI * fc/fs;
        bq_t sn = sin(omega);
        bq_t cs = cos(omega);
        bq_t alpha = sn * sinh(M_LN2 / 2.0 * bw * omega / sn);
        const float a0r = 1.0 / (1.0 + alpha);
        f->b0 = a0r * (1.0 - cs) * 0.5;
        f->b1 = a0r * (1.0 - cs);
        f->b2 = a0r * (1.0 - cs) * 0.5;
        f->a1 = a0r * (2.0 * cs);
        f->a2 = a0r * (alpha - 1.0);
}

static inline
void
hp_set_params(biquad *f, bq_t fc, bq_t bw, bq_t fs)
{
        bq_t omega = 2.0 * M_PI * fc/fs;
        bq_t sn = sin(omega);
        bq_t cs = cos(omega);
        bq_t alpha = sn * sinh(M_LN2 / 2.0 * bw * omega / sn);
        const float a0r = 1.0 / (1.0 + alpha);
        f->b0 = a0r * (1.0 + cs) * 0.5;
        f->b1 = a0r * -(1.0 + cs);
        f->b2 = a0r * (1.0 + cs) * 0.5;
        f->a1 = a0r * (2.0 * cs);
        f->a2 = a0r * (alpha - 1.0);
}

static inline
rev_t
biquad_run(biquad *f, rev_t x) {

	union {
	  rev_t y;
	  uint32_t y_int;
	} u;

        u.y = f->b0 * x + f->b1 * f->x1 + f->b2 * f->x2
		        + f->a1 * f->y1 + f->a2 * f->y2;
#ifdef REVERB_CALC_FLOAT
	if ((u.y_int & 0x7f800000) == 0)
	  u.y = 0.0f;
#endif
        f->x2 = f->x1;
        f->x1 = x;
        f->y2 = f->y1;
        f->y1 = u.y;

        return u.y;
}



typedef struct {
	float feedback;
	float fb_gain;
	float freq_resp;
	rev_t * ringbuffer;
	unsigned long buflen;
	unsigned long * buffer_pos;
	biquad * filter;
	rev_t last_out;
} COMB_FILTER;

typedef struct {
	float feedback;
	float fb_gain;
	float in_gain;
	rev_t * ringbuffer;
	unsigned long buflen;
	unsigned long * buffer_pos;
	rev_t last_out;
} ALLP_FILTER;


/* The structure used to hold port connection information and state */

typedef struct {
	unsigned long num_combs; /* total number of comb filters */
	unsigned long num_allps; /* total number of allpass filters */
	COMB_FILTER * combs;
	ALLP_FILTER * allps;
	biquad * low_pass; /* ptr to 2 low-pass filters */
	biquad * high_pass; /* ptr to 2 high-pass filters */
	unsigned long sample_rate;

	LADSPA_Data * decay;
	LADSPA_Data * drylevel;
	LADSPA_Data * wetlevel;
	LADSPA_Data * combs_en; /* on/off */
	LADSPA_Data * allps_en; /* on/off */
        LADSPA_Data * bandpass_en; /* on/off */
	LADSPA_Data * stereo_enh; /* on/off */
	LADSPA_Data * mode;

	LADSPA_Data * input_L;
	LADSPA_Data * output_L;
	LADSPA_Data * input_R;
	LADSPA_Data * output_R;

	LADSPA_Data old_decay;
	LADSPA_Data old_stereo_enh;
	LADSPA_Data old_mode;

	LADSPA_Data run_adding_gain;
} Reverb;

typedef struct {
	LADSPA_Data delay;
	LADSPA_Data feedback;
	LADSPA_Data freq_resp;
} COMB_DATA;

typedef struct {
	LADSPA_Data delay;
	LADSPA_Data feedback;
} ALLP_DATA;

typedef struct {
	unsigned long num_combs;
	unsigned long num_allps;
	COMB_DATA combs[MAX_COMBS];
	ALLP_DATA allps[MAX_ALLPS];
	LADSPA_Data bandpass_low;
	LADSPA_Data bandpass_high;
} REVERB_DATA;
