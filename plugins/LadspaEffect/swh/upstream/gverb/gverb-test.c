#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <bits/nan.h>

#include "gverb.h"

void run(const char *desc);

#define rdtscll(val) __asm__ __volatile__("rdtsc" : "=A" (val))
#define SIZE 48000

float in[SIZE], out[2][SIZE];
ty_gverb *verb;

int main(int argc, char *argv[])
{
	long long then, now;
	unsigned int i;
	float v;

	verb = gverb_new(48000, 300.0f, 50.0f, 7.0f, 0.5f, 15.0f, 0.5f, 0.5f, 0.5f);
	for (i=0; i<SIZE; i++) {
		in[i] = sin((double)i * 0.03);
	}
	run("slow sin");

	for (i=0; i<SIZE; i++) {
		in[i] = sin((double)i);
	}
	run("fast sin");

	v = 0.0000001535354543f;
	for (i=0; i<SIZE; i++) {
		in[i] = v;
		v *= 0.0001f;
	}
	run("small numbers");

	v = 1.0f;
	for (i=0; i<SIZE; i++) {
		in[i] = v;
		v *= 10.0f;
	}
	run("big numbers");

	for (i=0; i<SIZE/2; i++) {
		in[i] = sin((double)i);
	}
	for (; i<SIZE; i++) {
		in[i] = i % 50 * 0.0000000001543534f;
	}
	run("sin then small");

	for (i=0; i<SIZE/2; i++) {
		in[i] = sin((double)i);
	}
	for (; i<SIZE; i++) {
		in[i] = 0.0f;
	}
	run("sin then zero");

	for (i=0; i<SIZE; i++) {
		in[i] = 0.0f;
	}
	run("all zeros");

	for (i=0; i<SIZE; i++) {
		in[i] = 0.00000000432432f;
	}
	run("small constant");

	v = 1.0f;
	for (i=0; i<SIZE; i++) {
		if (random() > RAND_MAX / 5) {
			v *= -1.0f;
		}
		v *= -1.0f;
		in[i] = 1.0f;
	}
	run("+0dB pulse constant");

	for (i=0; i<SIZE; i++) {
		in[i] = 1.0e100f;
	}
	run("+lots dB constant");

	rdtscll(then);
	gverb_flush(verb);
	rdtscll(now);
	printf("flush took %lld cycles\n", now-then);

	return 0;
}

void run(const char *desc)
{
	unsigned int i;
	long long then, now;

	// total flush test: verb = gverb_new(48000, 300.0f, 50.0f, 7.0f, 0.5f, 15.0f, 0.5f, 0.5f, 0.5f);
	rdtscll(then);
	for (i=0; i<SIZE; i++) {
//printf("%f\n", in[i]);
		gverb_do(verb, in[i], out[0]+i, out[1]+i);
	}
	rdtscll(now);
	printf("%s took %lld cycles/sample\n", desc, (now - then) / (long long)SIZE);
	gverb_flush(verb);
}
