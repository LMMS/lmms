/*
 * DCblock
 *
 * This code has been extracted from the Csound opcode "dcblock".
 * It has been modified to work as a Soundpipe module.
 *
 * Original Author(s): Perry R. Cook
 * Year: 1995
 * Location: Opcodes/biquad.c
 *
 */

#include <math.h>
#include <stdlib.h>
#include "base.h"
#include "dcblock.h"

int sp_dcblock_create(sp_dcblock **p)
{
    *p = malloc(sizeof(sp_dcblock));
    return SP_OK;
}

int sp_dcblock_destroy(sp_dcblock **p)
{
    free(*p);
    return SP_OK;
}

int sp_dcblock_init(sp_data *sp, sp_dcblock *p, int oversampling )
{
    p->outputs = 0.0;
    p->inputs = 0.0;
    p->gain = pow( 0.99, 1.0f / oversampling );
    if (p->gain == 0.0 || p->gain>=1.0 || p->gain<=-1.0)
      p->gain = 0.99;
    return SP_OK;
}

int sp_dcblock_compute(sp_data *sp, sp_dcblock *p, SPFLOAT *in, SPFLOAT *out)
{
    SPFLOAT gain = p->gain;
    SPFLOAT outputs = p->outputs;
    SPFLOAT inputs = p->inputs;

    SPFLOAT sample = *in;
    outputs = sample - inputs + (gain * outputs);
    inputs = sample;
    *out = outputs;
    p->outputs = outputs;
    p->inputs = inputs;
    return SP_OK;
}
