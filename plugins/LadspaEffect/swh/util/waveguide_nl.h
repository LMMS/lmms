#ifndef WAVEGUIDE_NL_H
#define WAVEGUIDE_NL_H

#include <stdlib.h>
#include <string.h>

typedef struct {
	int size;
	float *buffer[2];
	int ptr;
	int delay;
	float fc;
	float lp[2];
	float a1a;
	float a1b;
	float zm1[2];
} waveguide_nl;

waveguide_nl *waveguide_nl_new(int size, float fc, float da, float db)
{
	waveguide_nl *wg = malloc(sizeof(waveguide_nl));
	wg->size = size;
	wg->delay = size;
	wg->buffer[0] = calloc(size, sizeof(float));
	wg->buffer[1] = calloc(size, sizeof(float));
	wg->ptr = 0;
	wg->fc = fc;
	wg->lp[0] = 0.0f;
	wg->lp[1] = 0.0f;
	wg->zm1[0] = 0.0f;
	wg->zm1[1] = 0.0f;
	wg->a1a = (1.0f - da) / (1.0f + da);
	wg->a1b = (1.0f - db) / (1.0f + db);

	return wg;
}

inline void waveguide_nl_reset(waveguide_nl *wg)
{
	memset(wg->buffer[0], 0, wg->size * sizeof(float));
	memset(wg->buffer[1], 0, wg->size * sizeof(float));
	wg->lp[0] = 0.0f;
	wg->lp[1] = 0.0f;
	wg->zm1[0] = 0.0f;
	wg->zm1[1] = 0.0f;
}

inline void waveguide_nl_free(waveguide_nl *wg)
{
	if (!wg) {
		return;
	}
	free(wg->buffer[0]);
	free(wg->buffer[1]);
	free(wg);
}

inline void waveguide_nl_set_delay(waveguide_nl *wg, int delay)
{
	if (delay > wg->size) {
		wg->delay = wg->size;
	} else if (delay < 1) {
		wg->delay = 1;
	} else {
		wg->delay = delay;
	}
}

inline void waveguide_nl_set_fc(waveguide_nl *wg, float fc)
{
	wg->fc = fc;
}

inline void waveguide_nl_set_ap(waveguide_nl *wg, float da, float db)
{
	wg->a1a = (1.0f - da) / (1.0f + da);
	wg->a1b = (1.0f - db) / (1.0f + db);
}

inline void waveguide_nl_process_lin(waveguide_nl *wg, float in0, float in1, float *out0, float *out1)
{
	float tmp;

	*out0 = wg->buffer[0][(wg->ptr + wg->delay) % wg->size];
	*out0 = wg->lp[0] * (wg->fc - 1.0f) + wg->fc * *out0;
	wg->lp[0] = *out0;
	tmp = *out0 * -(wg->a1a) + wg->zm1[0];
	wg->zm1[0] = tmp * wg->a1a + *out0;
	*out0 = tmp;

	*out1 = wg->buffer[1][(wg->ptr + wg->delay) % wg->size];
	*out1 = wg->lp[1] * (wg->fc - 1.0f) + wg->fc * *out1;
	wg->lp[1] = *out1;
	tmp = *out1 * -(wg->a1a) + wg->zm1[1];
	wg->zm1[1] = tmp * wg->a1a + *out1;
	*out1 = tmp;

	wg->buffer[0][wg->ptr] = in0;
	wg->buffer[1][wg->ptr] = in1;
	wg->ptr--;
	if (wg->ptr < 0) {
		wg->ptr += wg->size;
	}
}

inline void waveguide_nl_process(waveguide_nl *wg, float in0, float in1, float *out0, float *out1)
{
	float tmp;
	float a1;
	float b;

	*out0 = wg->buffer[0][(wg->ptr + wg->delay) % wg->size];
	*out0 = wg->lp[0] * (wg->fc - 1.0f) + wg->fc * *out0;
	wg->lp[0] = *out0;
	b = (*out0 + 1.0) * 6.0f;
	if (b > 1.0f) {
		b = 1.0f;
	}
	if (b < 0.0f) {
		b = 0.0f;
	}
	a1 = b * wg->a1a + (1.0f - b) * wg->a1b;
	tmp = *out0 * -a1 + wg->zm1[0];
	wg->zm1[0] = tmp * a1 + *out0;
	*out0 = tmp;

	*out1 = wg->buffer[1][(wg->ptr + wg->delay) % wg->size];
	*out1 = wg->lp[1] * (wg->fc - 1.0f) + wg->fc * *out1;
	wg->lp[1] = *out1;
	b = (*out1 + 1.0) * 6.0f;
	if (b > 1.0f) {
		b = 1.0f;
	}
	if (b < 0.0f) {
		b = 0.0f;
	}
	a1 = b * wg->a1a + (1.0f - b) * wg->a1b;
	tmp = *out1 * -a1 + wg->zm1[1];
	wg->zm1[1] = tmp * a1 + *out1;
	*out1 = tmp;

	wg->buffer[0][wg->ptr] = in0;
	wg->buffer[1][wg->ptr] = in1;
	wg->ptr--;
	if (wg->ptr < 0) {
		wg->ptr += wg->size;
	}
}

#endif
