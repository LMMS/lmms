/*
	dsp/Delay.h
	
	Copyright 2003-4 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	delay lines with fractional (linear or cubica interpolation) lookup 
	and an allpass interpolating tap (which needs more work).

	delay line storage is aligned to powers of two for simplified wrapping
	checks (no conditional or modulo, binary and suffices instead).

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

#ifndef _DSP_DELAY_H_
#define _DSP_DELAY_H_

#include "util.h"
#include "FPTruncateMode.h"

namespace DSP {
	
class Delay
{
	public:
		int size;
		d_sample * data;
		int read, write;

		Delay()
			{
				read = write = 0;
				data = 0;
			}

		~Delay()
			{
				if (data) free (data);
			}

		void init (int n)
			{
				size = next_power_of_2 (n);
				data = (d_sample *) calloc (sizeof (d_sample), size);
				size -= 1;
				write = n;
			}

		void reset()
			{
				memset (data, 0, (size + 1) * sizeof (d_sample));
			}

		d_sample & 
		operator [] (int i)
			{
				return data [(write - i) & size];
			}

		inline void 
		put (d_sample x)
			{
				data [write] = x;
				write = (write + 1) & size;
			}

		inline d_sample 
		get()
			{
				d_sample x = data [read];
				read = (read + 1) & size;
				return x;
			}

		inline d_sample
		putget (d_sample x)
			{
				put (x);
				return get();
			}

		/* fractional lookup, linear interpolation */
		inline d_sample 
		get_at (float f)
			{
				int n;
				fistp (f, n); /* read: i = (int) f; relies on FPTruncateMode */
				f -= n;
				
				return (1 - f) * (*this) [n] + f * (*this) [n + 1];
			}

		/* fractional lookup, cubic interpolation */
		inline d_sample
		get_cubic (float f)
			{
				int n;
				fistp (f, n); /* see FPTruncateMode */
				f -= n;

				d_sample x_1 = (*this) [n - 1];
				d_sample x0 = (*this) [n];
				d_sample x1 = (*this) [n + 1];
				d_sample x2 = (*this) [n + 2];

				/* d_sample (32bit) quicker than double here */
				register d_sample a = 
						(3 * (x0 - x1) - x_1 + x2) * .5;
				register d_sample b =
						2 * x1 + x_1 - (5 * x0 + x2) * .5;
				register d_sample c = 
						(x1 - x_1) * .5;

				return x0 + (((a * f) + b) * f + c) * f;
			}
};

/* allpass variant */

class DelayTapA
{
	public:
		d_sample x1, y1;

		DelayTapA()
			{
				reset();
			}

		void reset()
			{
				x1 = y1 = 0;
			}

		d_sample get (Delay & d, float f)
			{
				int n;
				fistp (f, n); /* read: i = (int) f; relies on FPTruncateMode */
				f -= n;
				if (0 && f < .5)
					f += 1,
					n -= 1;

				d_sample x = d[n];
				f = (1 - f) / (1 + f);
				y1 = x1 + f * x - f * y1;
				x1 = x;
				return y1;
			}
};

}; /* namespace DSP */

#endif /* _DSP_DELAY_H_ */
