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

    $Id: tap_utils.h,v 1.5 2004/02/21 17:33:36 tszilagyi Exp $
*/
#ifndef _ISOC99_SOURCE
#define _ISOC99_SOURCE
#endif

#include <stdint.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327
#endif



/* push a sample into a ringbuffer and return the sample falling out */
static inline
LADSPA_Data
push_buffer(LADSPA_Data insample, LADSPA_Data * buffer,
            unsigned long buflen, unsigned long * pos) {

        LADSPA_Data outsample;

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
LADSPA_Data
read_buffer(LADSPA_Data * buffer, unsigned long buflen,
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
write_buffer(LADSPA_Data insample, LADSPA_Data * buffer, unsigned long buflen,
             unsigned long pos, unsigned long n) {

        while (n + pos >= buflen)
                n -= buflen;
        buffer[n + pos] = insample;
}




/* Please note that the majority of the definitions and helper
functions below have been derived from the source code of Steve
Harris's SWH plugins (particularly from the "biquad.h" file).  While I
give him credit for his excellent work, I reserve myself to be blamed
for any bugs or malfunction. */


#define db2lin(x) ((x) > -90.0f ? powf(10.0f, (x) * 0.05f) : 0.0f)

#define ABS(x)  (x)>0.0f?(x):-1.0f*(x)


#define LN_2_2 0.34657359f
#define LIMIT(v,l,u) ((v)<(l)?(l):((v)>(u)?(u):(v)))

#define BIQUAD_TYPE float
typedef BIQUAD_TYPE bq_t;


/* Biquad filter (adapted from lisp code by Eli Brandt,
   http://www.cs.cmu.edu/~eli/) */

/* The prev. comment has been preserved from Steve Harris's biquad.h */

typedef struct {
	bq_t a1;
	bq_t a2;
	bq_t b0;
	bq_t b1;
	bq_t b2;
	bq_t x1;
	bq_t x2;
	bq_t y1;
	bq_t y2;
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
#if 0
b0 = (1 - cs) /2;
b1 = 1 - cs;
b2 = (1 - cs) /2;
a0 = 1 + alpha;
a1 = -2 * cs;
a2 = 1 - alpha;
#endif
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

#if 0
b0 = (1 + cs) /2;
b1 = -(1 + cs);
b2 = (1 + cs) /2;
a0 = 1 + alpha;
a1 = -2 * cs;
a2 = 1 - alpha;
#endif
        f->b0 = a0r * (1.0 + cs) * 0.5;
        f->b1 = a0r * -(1.0 + cs);
        f->b2 = a0r * (1.0 + cs) * 0.5;
        f->a1 = a0r * (2.0 * cs);
        f->a2 = a0r * (alpha - 1.0);
}


static inline
void
ls_set_params(biquad *f, bq_t fc, bq_t gain, bq_t slope, bq_t fs)
{

	bq_t w = 2.0f * M_PI * LIMIT(fc, 1.0, fs/2.0) / fs;
	bq_t cw = cosf(w);
	bq_t sw = sinf(w);
	bq_t A = powf(10.0f, gain * 0.025f);
	bq_t b = sqrt(((1.0f + A * A) / LIMIT(slope, 0.0001f, 1.0f)) - ((A -
					1.0f) * (A - 1.0)));
	bq_t apc = cw * (A + 1.0f);
	bq_t amc = cw * (A - 1.0f);
	bq_t bs = b * sw;
	bq_t a0r = 1.0f / (A + 1.0f + amc + bs);

	f->b0 = a0r * A * (A + 1.0f - amc + bs);
	f->b1 = a0r * 2.0f * A * (A - 1.0f - apc);
	f->b2 = a0r * A * (A + 1.0f - amc - bs);
	f->a1 = a0r * 2.0f * (A - 1.0f + apc);
	f->a2 = a0r * (-A - 1.0f - amc + bs);
}


static inline
void
hs_set_params(biquad *f, bq_t fc, bq_t gain, bq_t slope, bq_t fs) {

	bq_t w = 2.0f * M_PI * LIMIT(fc, 1.0, fs/2.0) / fs;
	bq_t cw = cosf(w);
	bq_t sw = sinf(w);
	bq_t A = powf(10.0f, gain * 0.025f);
	bq_t b = sqrt(((1.0f + A * A) / LIMIT(slope, 0.0001f, 1.0f)) - ((A -
					1.0f) * (A - 1.0f)));
	bq_t apc = cw * (A + 1.0f);
	bq_t amc = cw * (A - 1.0f);
	bq_t bs = b * sw;
	bq_t a0r = 1.0f / (A + 1.0f - amc + bs);

	f->b0 = a0r * A * (A + 1.0f + amc + bs);
	f->b1 = a0r * -2.0f * A * (A - 1.0f + apc);
	f->b2 = a0r * A * (A + 1.0f + amc - bs);
	f->a1 = a0r * -2.0f * (A - 1.0f - apc);
	f->a2 = a0r * (-A - 1.0f + amc + bs);
}


static inline
bq_t
biquad_run(biquad *f, bq_t x) {

	union {
	  bq_t y;
	  uint32_t y_int;
	} u;
	
	u.y = f->b0 * x + f->b1 * f->x1 + f->b2 * f->x2
		        + f->a1 * f->y1 + f->a2 * f->y2;
	if ((u.y_int & 0x7f800000) == 0)
	  u.y = 0.0f;
	f->x2 = f->x1;
	f->x1 = x;
	f->y2 = f->y1;
	f->y1 = u.y;

	return u.y;
}
