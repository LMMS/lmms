/*
	Eq.h
	
	Copyright 2004-7 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	Equalizer circuit using recursive filtering.
	Based on a motorola paper implementing a similar circuit on a DSP56001.

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

#ifndef _DSP_EQ_H_
#define _DSP_EQ_H_

namespace DSP {

/* A single bandpass as used by the Eq, expressed as a biquad. Like all
 * band-pass filters I know of, the filter works with a FIR coefficient of 0 
 * for x[-1], so a generic biquad isn't the optimum implementation. 
 *
 * This routine isn't used anywhere, just here for testing purposes.
 */
template <class T>
void
_BP (double fc, double Q, T * ca, T * cb)
{
	double theta = 2 * fc * M_PI;

	double 
		b = (Q - theta * .5) / (2 * Q + theta),
		a = (.5 - b) / 2,
		c = (.5 + b) * cos (theta);

	ca[0] = 2 * a;
	ca[1] = 0;
	ca[2] = -2 * a;

	cb[0] = 0;
	cb[1] = 2 * c;
	cb[2] = -2 * b;
}

template <int Bands, class eq_sample = float>
class Eq
{
	public:
		/* recursion coefficients, 3 per band */
		eq_sample __attribute__ ((aligned)) a[Bands], b[Bands], c[Bands];
		/* past outputs, 2 per band */
		eq_sample __attribute__ ((aligned)) y[2][Bands];
		/* current gain and recursion factor, each 1 per band = 2 */
		eq_sample __attribute__ ((aligned)) gain[Bands], gf[Bands];
		/* input history */
		eq_sample x[2];
		/* history index */
		int h;

		eq_sample normal;

		Eq()
			{
				h = 0;
				normal = NOISE_FLOOR;
			}

		void reset()
			{
				for (int z = 0; z < 2; ++z)
				{
					memset( y[z], 0, Bands*sizeof( eq_sample ) );
					x[z] = 0;
				}
			}

		void init (double fs, double Q)
			{
				double f = 31.25;
				int i = 0;

				for (i = 0; i < Bands && f < fs / 2; ++i, f *= 2)
					init_band (i, 2 * f * M_PI / fs, Q);
				/* just in case, zero the remaining coefficients */
				for (  ; i < Bands; ++i)
					zero_band (i);

				reset();
			}	

		void init_band (int i, double theta, double Q)
			{
				b[i] = (Q - theta * .5) / (2 * Q + theta);
				a[i] = (.5 - b[i]) / 2;
				c[i] = (.5 + b[i]) * cos (theta);
				/* fprintf (stderr, "%02d %f %f %f\n", i, a[i], b[i], c[i]); */
				gain[i] = 1;
				gf[i] = 1;
			}

		void zero_band (int i)
			{
				a[i] = b[i] = c[i] = 0;
			}

		/* per-band recursion:
		 * 	y = 2 * (a * (x - x[-2]) + c * y[-1] - b * y[-2]) 
		 */
		eq_sample process (eq_sample s)
			{
				int z1 = h, z2 = h ^ 1;

				eq_sample * y1 = y[z1];
				eq_sample * y2 = y[z2];

				eq_sample x_x2 = s - x[z2];
				eq_sample r = 0;

				for (int i = 0; i < Bands; ++i)
				{
					y2[i] = normal + 2 * (a[i] * x_x2 + c[i] * y1[i] - b[i] * y2[i]);
					r += gain[i] * y2[i];
					gain[i] *= gf[i];
				}

				x[z2] = s;
				h = z2;

				return r;
			}

		/* zap denormals in history */
		void flush_0()
			{
				for (int i = 0; i < Bands; ++i)
					if (is_denormal (y[0][i]))
						y[0][i] = 0;
			}
};

} /* namespace DSP */

#endif /* _DSP_EQ_H_ */
