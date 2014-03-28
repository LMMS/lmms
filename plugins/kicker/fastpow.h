
#ifndef FASTPOW_H
#define FASTPOW_H

#include <stdint.h>

/*
 * source:
 * http://martin.ankerl.com/2007/10/04/optimized-pow-approximation-for-java-and-c-c/
 */

double fastPow(double a, double b) {
  union {
    double d;
    int32_t x[2];
  } u = { a };
  u.x[1] = (int32_t)(b * (u.x[1] - 1072632447) + 1072632447);
  u.x[0] = 0;
  return u.d;
}

#endif
