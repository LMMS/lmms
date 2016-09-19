#ifndef _BUFFER_H
#define _BUFFER_H

/* substract buffer b from a, save in c
 *
 * this could be sped up by vector operations
 */

static inline void buffer_sub(const float* a, const float *b, float *c, int cnt) {
	int i;
	for(i=0;i<cnt;++i)
		*c++ = *a++ - *b++;
}

#endif
