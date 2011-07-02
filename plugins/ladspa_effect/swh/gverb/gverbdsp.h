
#ifndef GVERBDSP_H
#define GVERBDSP_H

#include "../ladspa-util.h"

typedef struct {
  int size;
  int idx;
  float *buf;
} ty_fixeddelay;

typedef struct {
  int size;
  float coeff;
  int idx;
  float *buf;
} ty_diffuser;

typedef struct {
  float damping;
  float delay;
} ty_damper;

ty_diffuser *diffuser_make(int, float);
void diffuser_free(ty_diffuser *);
void diffuser_flush(ty_diffuser *);
//float diffuser_do(ty_diffuser *, float);

ty_damper *damper_make(float);
void damper_free(ty_damper *);
void damper_flush(ty_damper *);
//void damper_set(ty_damper *, float);
//float damper_do(ty_damper *, float);

ty_fixeddelay *fixeddelay_make(int);
void fixeddelay_free(ty_fixeddelay *);
void fixeddelay_flush(ty_fixeddelay *);
//float fixeddelay_read(ty_fixeddelay *, int);
//void fixeddelay_write(ty_fixeddelay *, float);

int isprime(int);
int nearest_prime(int, float);

static inline float diffuser_do(ty_diffuser *p, float x)
{
  float y,w;

  w = x - p->buf[p->idx]*p->coeff;
  w = flush_to_zero(w);
  y = p->buf[p->idx] + w*p->coeff;
  p->buf[p->idx] = w;
  p->idx = (p->idx + 1) % p->size;
  return(y);
}

static inline float fixeddelay_read(ty_fixeddelay *p, int n)
{
  int i;

  i = (p->idx - n + p->size) % p->size;
  return(p->buf[i]);
}

static inline void fixeddelay_write(ty_fixeddelay *p, float x)
{
  p->buf[p->idx] = x;
  p->idx = (p->idx + 1) % p->size;
}

static inline void damper_set(ty_damper *p, float damping)
{ 
  p->damping = damping;
} 
  
static inline float damper_do(ty_damper *p, float x)
{ 
  float y;
    
  y = x*(1.0-p->damping) + p->delay*p->damping;
  p->delay = y;
  return(y);
}

#endif
