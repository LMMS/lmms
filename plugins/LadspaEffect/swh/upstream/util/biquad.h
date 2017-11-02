#ifndef BIQUAD_H
#define BIQUAD_H

#define LN_2_2 0.34657359f // ln(2)/2

#include "ladspa-util.h"

#ifndef LIMIT
#define LIMIT(v,l,u) (v<l?l:(v>u?u:v))
#endif

#ifndef BIQUAD_TYPE
#define BIQUAD_TYPE float
#endif

typedef BIQUAD_TYPE bq_t;

/* Biquad filter (adapted from lisp code by Eli Brandt,
   http://www.cs.cmu.edu/~eli/) */

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

static inline void eq_set_params(biquad *f, bq_t fc, bq_t gain, bq_t bw,
			  bq_t fs);
static inline void eq_set_params(biquad *f, bq_t fc, bq_t gain, bq_t bw,
			  bq_t fs)
{
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

static inline void ls_set_params(biquad *f, bq_t fc, bq_t gain, bq_t slope,
			  bq_t fs);
static inline void ls_set_params(biquad *f, bq_t fc, bq_t gain, bq_t slope,
			  bq_t fs)
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

static inline void hs_set_params(biquad *f, bq_t fc, bq_t gain, bq_t slope,
			  bq_t fs);
static inline void hs_set_params(biquad *f, bq_t fc, bq_t gain, bq_t slope,
			  bq_t fs)
{
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

static inline void lp_set_params(biquad *f, bq_t fc, bq_t bw, bq_t fs)
{
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

static inline void hp_set_params(biquad *f, bq_t fc, bq_t bw, bq_t fs)
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

static inline void bp_set_params(biquad *f, bq_t fc, bq_t bw, bq_t fs)
{
	bq_t omega = 2.0 * M_PI * fc/fs;
	bq_t sn = sin(omega);
	bq_t cs = cos(omega);
	bq_t alpha = sn * sinh(M_LN2 / 2.0 * bw * omega / sn);

        const float a0r = 1.0 / (1.0 + alpha);
        f->b0 = a0r * alpha;
        f->b1 = 0.0;
        f->b2 = a0r * -alpha;
        f->a1 = a0r * (2.0 * cs);
        f->a2 = a0r * (alpha - 1.0);
}

static inline bq_t biquad_run(biquad *f, const bq_t x) {
	bq_t y;

	y = f->b0 * x + f->b1 * f->x1 + f->b2 * f->x2
		      + f->a1 * f->y1 + f->a2 * f->y2;
	y = flush_to_zero(y);
	f->x2 = f->x1;
	f->x1 = x;
	f->y2 = f->y1;
	f->y1 = y;

	return y;
}

static inline bq_t biquad_run_fb(biquad *f, bq_t x, const bq_t fb) {
	bq_t y;

	x += f->y1 * fb * 0.98;
	y = f->b0 * x + f->b1 * f->x1 + f->b2 * f->x2
		      + f->a1 * f->y1 + f->a2 * f->y2;
	y = flush_to_zero(y);
	f->x2 = f->x1;
	f->x1 = x;
	f->y2 = f->y1;
	f->y1 = y;

	return y;
}

#endif
