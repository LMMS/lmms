/*
	Eq.h
	
	Copyright 2004 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	equalizer circuit using recursive filtering.
	based on a motorola paper implementing a similar circuit on a DSP56001.

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

/* a single bandpass as used by the Eq, expressed as a biquad. like all
 * band-pass filters i know, the filter works with a FIR coefficient of 0 
 * for x[-1], so a generic biquad isn't the optimum implementation. 
 */
class BP
{
	public:
		template <class T>
		BP (double fc, double Q, T * ca, T * cb)
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
};

/* BANDS must be a multiple of 4 to enable the use of SSE instructions.
 * however, the current SSE-enabled process() method fails to compile if
 * -funroll-loops is passed to gcc.
 */
template <int USE_BANDS, int BANDS>
class Eq
{
	public:
		/* over-size state buffer to allow alignment */
		float state [BANDS * 8 + 4 + 4];
		/* recursion coefficients, 3 per band */
		float * a, * b, * c;
		/* past outputs, 2 per band */
		float * y;
		/* current gain and recursion factor, each 1 per band = 2 */
		float * gain, * gf;
		/* aligned storage for output summation */
		float * temp;
		/* aligned storage for constants */
		float * two;
		/* input history */
		float x[2];
		/* history index */
		int h;

		Eq()
			{
				/* take care of 128-bit alignment */
				long s = (long) (char *) state;
				s &= 0xF;
				if (s)
					s = 16 - s;
				
				/* assign coefficients */
				a = (float *) (((char *) state) + s); 
				b = a + BANDS; 
				c = a + 2 * BANDS; 
				
				/* output history (input is common to all bands) */
				y = a + 3 * BANDS; 
				gain = a + 5 * BANDS; 
				gf = a + 6 * BANDS; 
				temp = a + 7 * BANDS;
				two = temp + 4;
				two[0] = two[1] = two[2] = two[3] = 2;

				h = 0;
			}

		void reset()
			 {
				 for (int i = 0; i < 2 * BANDS; ++i)
					y[i] = 0;

				for (int i = 0; i < 2; ++i)
					x[i] = 0;
			}

		void init (double fs, double Q)
			{
				double f = 31.25;
				int i = 0;

				for (i = 0; i < USE_BANDS && f < fs / 2; ++i, f *= 2)
					init_band (i, 2 * f * M_PI / fs, Q);
				for (  ; i < BANDS; ++i)
					zero_band (i);

				reset();
			}	

		void init_band (int i, double theta, double Q)
			{
				b[i] = (Q - theta * .5) / (2 * Q + theta);
				a[i] = (.5 - b[i]) / 2;
				c[i] = (.5 + b[i]) * cos (theta);
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
		d_sample process (d_sample s)
			{
				int z1 = h, z2 = h ^ 1;

				float * y1 = y + z1 * BANDS;
				float * y2 = y + z2 * BANDS;

				d_sample x_x2 = s - x[z2];
				d_sample r = 0;

				for (int i = 0; i < USE_BANDS; ++i)
				{
					y2[i] = 2 * (a[i] * x_x2 + c[i] * y1[i] - b[i] * y2[i]);
					r += gain[i] * y2[i];
					gain[i] *= gf[i];
				}
				
				x[z2] = s;
				h = z2;

				return r;
			}
};

} /* namespace DSP */

#endif /* _DSP_EQ_H_ */
