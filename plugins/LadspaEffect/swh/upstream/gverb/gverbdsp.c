

/*

        Copyright (C) 1999 Juhana Sadeharju
                       kouhia at nic.funet.fi

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

    */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "gverbdsp.h"

#define TRUE 1
#define FALSE 0

ty_diffuser *diffuser_make(int size, float coeff)
{
  ty_diffuser *p;
  int i;

  p = (ty_diffuser *)malloc(sizeof(ty_diffuser));
  p->size = size;
  p->coeff = coeff;
  p->idx = 0;
  p->buf = (float *)malloc(size*sizeof(float));
  for (i = 0; i < size; i++) p->buf[i] = 0.0;
  return(p);
}

void diffuser_free(ty_diffuser *p)
{
  free(p->buf);
  free(p);
}

void diffuser_flush(ty_diffuser *p)
{
  memset(p->buf, 0, p->size * sizeof(float));
}

ty_damper *damper_make(float damping)
{
  ty_damper *p;

  p = (ty_damper *)malloc(sizeof(ty_damper));
  p->damping = damping;
  p->delay = 0.0f;
  return(p);
}

void damper_free(ty_damper *p)
{
  free(p);
}

void damper_flush(ty_damper *p)
{
  p->delay = 0.0f;
}

ty_fixeddelay *fixeddelay_make(int size)
{
  ty_fixeddelay *p;
  int i;

  p = (ty_fixeddelay *)malloc(sizeof(ty_fixeddelay));
  p->size = size;
  p->idx = 0;
  p->buf = (float *)malloc(size*sizeof(float));
  for (i = 0; i < size; i++) p->buf[i] = 0.0;
  return(p);
}

void fixeddelay_free(ty_fixeddelay *p)
{
  free(p->buf);
  free(p);
}

void fixeddelay_flush(ty_fixeddelay *p)
{
  memset(p->buf, 0, p->size * sizeof(float));
}

int isprime(int n)
{
  unsigned int i;
  const unsigned int lim = (int)sqrtf((float)n);

  if (n == 2) return(TRUE);
  if ((n & 1) == 0) return(FALSE);
  for(i = 3; i <= lim; i += 2)
    if ((n % i) == 0) return(FALSE);
  return(TRUE);
}

int nearest_prime(int n, float rerror)
     /* relative error; new prime will be in range
      * [n-n*rerror, n+n*rerror];
      */
{
  int bound,k;

  if (isprime(n)) return(n);
  /* assume n is large enough and n*rerror enough smaller than n */
  bound = n*rerror;
  for(k = 1; k <= bound; k++) {
    if (isprime(n+k)) return(n+k);
    if (isprime(n-k)) return(n-k);
  }
  return(-1);
}
