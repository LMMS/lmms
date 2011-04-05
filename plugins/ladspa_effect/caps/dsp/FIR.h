/*
	dsp/FIR.h
	
	Copyright 2003-10 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	finite impulse response filters, with options for up- and down-sampling.

*/
/*
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA or point your web browser to http://www.gnu.org.
*/

#ifndef _FIR_H_
#define _FIR_H_

#include "util.h"

namespace DSP {
	
/* brute-force FIR filter with downsampling method (decimating). 
 *
 * CAVEAT: constructing it from another FIR makes the filter share the other's 
 * kernel data set. IOW, the other FIR must be valid throughout the lifetime 
 * of this instance.
 */
class FIR
{
	public:
		/* kernel length, history length - 1 */
		int n, m;
		
		/* coefficients, history */
		sample_t * c, * x;
		bool borrowed_kernel;

		/* history index */
		int h; 
		
		FIR (int N)
			{
				c = 0;
				init (N);
			}

		FIR (FIR & fir)
			{
				c = fir.c;
				init (fir.n);
			}

		FIR (int n, sample_t * kernel)
			{
				c = 0;
				init (n);
				memcpy (c, kernel, n * sizeof (*c));
			}

		~FIR()
			{
				if (!borrowed_kernel)
					free (c);
				free (x);
			}
		
		void init (int N)
			{
				n = N;

				/* keeping history size a power of 2 makes it possible to wrap the
				 * history pointer by & instead of %, saving a few cpu cycles. */
				m = next_power_of_2 (n);

				if (c)
					borrowed_kernel = true;
				else
					borrowed_kernel = false,
					c = (sample_t *) malloc (n * sizeof (sample_t));

				x = (sample_t *) malloc (m * sizeof (sample_t));

				m -= 1;

				reset();
			}
	
		void reset()
			{
				h = 0;
				memset (x, 0, n * sizeof (sample_t));
			}
		
		/* TODO: write an SSE-enabled version */
		inline sample_t process (sample_t s)
			{
				x[h] = s;
				
				s *= c[0];

				for (int Z = 1, z = h - 1; Z < n; --z, ++Z)
					s += c[Z] * x[z & m];

				h = (h + 1) & m;

				return s;
			}

		/* Z is the time, in samples, since the last non-zero sample.
		 * OVER is the oversampling factor. just here for documentation, use
		 * a FIRUpsampler instead.
		 */
		template <int Z, int OVER>
		inline sample_t upsample (sample_t s)
			{
				x[h] = s;
				
				s = 0;

				/* for the interpolation, iterate over the history in z ^ -OVER
				 * steps -- all the samples between are 0.
				 */
				for (int j = Z, z = h - Z; j < n; --z, j += OVER)
					s += c[j] * x[z & m];

				h = (h + 1) & m;

				return s;
			}

		/* used in downsampling */
		inline void store (sample_t s)
			{
				x[h] = s;
				h = (h + 1) & m;
			}
};

/* close relative of FIR, but distinct enough to not justify inheritance.
 * 
 * the difference to the FIR is the shorter history length. don't need
 * to clutter the d-cache with interleaved 0s.
 *
 * however, an initial test shows this to be a fraction *slower* than a
 * complete FIR for N = 32, OVER = 4.
 */
class FIRUpsampler
{
	public:
		/* kernel length, history length - 1 */
		int n, m;

		/* oversampling ratio */
		int over;
		
		/* coefficients, history */
		sample_t * c, * x;

		/* history index */
		int h; 
		
		FIRUpsampler (int _n, int _over)
			{
				c = x = 0;
				init (_n, _over);
			}

		FIRUpsampler (FIR & fir, int _over)
			{
				c = x = 0;
				init (fir.n, _over);
				memcpy (c, fir.c, n * sizeof (sample_t));
			}

		~FIRUpsampler()
			{
				if (c) free (c);
				if (x) free (x);
			}
		
		void init (int _n, int _over)
			{
				/* oversampling ratio must be multiple of FIR kernel length */
				// assert (_n % _over == 0);

				n = _n;
				over = _over;

				/* like FIR, keep the history buffer a power of 2; additionally,
				 * compress and don't store the 0 samples inbetween.
				 */
				m = next_power_of_2 ((n + over - 1) / over);

				c = (sample_t *) malloc (n * sizeof (sample_t));
				x = (sample_t *) malloc (m * sizeof (sample_t));

				m -= 1;

				reset();
			}
	
		void reset()
			{
				h = 0;
				memset (x, 0, (m + 1) * sizeof (sample_t));
			}
		
		/* upsample the given sample */
		inline sample_t upsample (sample_t s)
			{
				x[h] = s;
				
				s = 0;

				for (int Z = 0, z = h; Z < n; --z, Z += over)
					s += c[Z] * x[z & m];

				h = (h + 1) & m;

				return s;
			}

		/* upsample a zero sample (interleaving), Z being the time, in samples,
		 * since the last non-0 sample. */
		inline sample_t pad (int Z)
			{
				sample_t s = 0;

				for (int z = h - 1; Z < n; --z, Z += over)
					s += c[Z] * x[z & m];

				return s;
			}

};

}; /* namespace DSP */

#endif /* _FIR_H_ */
