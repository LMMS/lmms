/*
	dsp/Sine.h
	
	Copyright 2003-4 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	direct form I recursive sin() generator.

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

#ifndef _DSP_SINE_H_
#define _DSP_SINE_H_

namespace DSP {
	
class Sine
{
	protected:
		int z;
		d_float y[2];
		d_float b;

	public:
		Sine()
			{ 
				b = 0;
				y[0] = y[1] = 0;
				z = 0;
			}

		Sine (double f, double fs, double phase)
			{
				set_f (f, fs, phase);
			}

		Sine (double omega, double phase = 0.)
			{
				set_f (omega, phase);
			}

		inline void set_f (double f, double fs, double phase)
			{
				set_f (f * M_PI / fs, phase);
			}

		inline void set_f (double w, double phase)
			{
				b = 2 * cos (w);
				y[0] = sin (phase - w);
				y[1] = sin (phase - w * 2);
				z = 0;
			}

		/* advance and return 1 sample */
		inline double get()
			{
				register double s = b * y[z]; 
				z ^= 1;
				s -= y[z];
				return y[z] = s;
			}

		double get_phase()
			{
				double x0 = y[z], x1 = b * y[z] - y[z^1];
				double phi = asin (x0);
				
				/* slope is falling, we're into the 2nd half. */
				if (x1 < x0)
					return M_PI - phi;

				return phi;
			}
};

} /* namespace DSP */

#endif /* _DSP_SINE_H_ */
