/*
	dsp/OnePole.h
	
	Copyright 2003-7 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	one pole (or one zero, or one zero, one pole) hi- and lo-pass filters.

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

#ifndef _ONE_POLE_H_
#define _ONE_POLE_H_

namespace DSP {
	
class OnePoleLP
{
	public:
		sample_t a0, b1, y1;

		OnePoleLP (double d = 1.)
			{
				set (d);
				y1 = 0.;
			}

		inline void reset()
			{
				y1 = 0.;
			}

		inline void set_f (double fc)
			{
				set (exp (-2 * M_PI * fc));
			}

		inline void set (double d)
			{
				a0 = (sample_t) d;
				b1 = (sample_t) 1. - d;
			}

		inline sample_t process (sample_t x)
			{
				return y1 = a0 * x + b1 * y1;
			}
		
		inline void decay (double d)
			{
				a0 *= d;
				b1 = 1. - a0;
			}

		/* clear denormal numbers in history */
		void flush_0()
			{
				if (is_denormal (y1))
					y1 = 0;
			}
};

class OnePoleHP
{
	public:
		sample_t a0, a1, b1, x1, y1;

		OnePoleHP (double d = 1.)
			{
				set (d);
				x1 = y1 = 0.;
			}

		void set_f (double f)
			{
				set (exp (-2 * M_PI * f));
			}

		inline void set (double d)
			{
				a0 = (sample_t) ((1. + d) / 2.);
				a1 = (sample_t) ((1. + d) / -2.);
				b1 = d;
			}

		inline sample_t process (sample_t x)
			{
				y1 = a0 * x + a1 * x1 + b1 * y1;
				x1 = x;
				return y1;
			}

		void reset()
			{
				x1 = y1 = 0;
			}

		/* clear denormal numbers in history */
		void flush_0()
			{
				if (is_denormal (y1))
					y1 = 0;
			}
};

} /* namespace DSP */

#endif /* _ONE_POLE_H_ */
