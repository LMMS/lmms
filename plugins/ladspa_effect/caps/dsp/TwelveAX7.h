/*
	dsp/TwelveAX7.h
	
	Copyright 2003-6 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	collection of approximations of the 12AX7 voltage transfer function
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

#ifndef _DSP_TWELVE_AX_7_H_
#define _DSP_TWELVE_AX_7_H_

namespace DSP {
	
#include "r12ax7.h"

typedef d_sample tube_sample;

/* this is the original tube model from caps < 0.1.9 or preamp.so, put
 * back into use in 0.1.11; the replacement (below) is too strong in 
 * odd-order harmonics at the expense of even-order. it has to sound
 * good: it took a good deal of fiddling to get the coefficients right.
 */
class TwelveAX7
{
	public:
		tube_sample b, c, d;
		
		struct {
			tube_sample threshold, value;
		} clip[2];

		/* amplitude at which clipping starts */
		tube_sample scale;

	public:
		TwelveAX7()
			{ 
				/* transfer polynomial parameters */
				b = -0.79618574210627535;
				c = -0.21108555430962023;
				d = +0.38944033523200522;

				set_clips();

				scale = min (fabs (clip[0].threshold), fabs (clip[1].threshold));
			}

		inline tube_sample transfer (tube_sample a)
			{ 
				return a * (b + a * (c + a * d)); 
			}
			
		inline tube_sample transfer_clip (tube_sample a)
			{
				if (a <= clip[0].threshold)
					return clip[0].value;
				if (a >= clip[1].threshold)
					return clip[1].value;
				return transfer (a);
			}

		inline double get_root (double sign)
			{
				/* only once, no need to optimize */
				return 
					(-2*c + sign * sqrt ((2*c) * (2*c) - 4 * (3 * d * b))) / (6 * d);
			}

		inline void set_clips()
			{
				/* find 0 crossings in the derived, this is where we'll clip */
				double x0 = get_root (-1);
				double x1 = get_root (+1);

				clip[0].value = transfer (x0);
				clip[1].value = transfer (x1);

				clip[0].threshold = x0;
				clip[1].threshold = x1;
			}
};

/* reworked model. higher order (than 3) polynomials make little sense;
 * sonically the difference is minim, and the cycle count increases
 * dramatically.
 */
class TwelveAX7_2
{
	public:
		tube_sample b, c, d;

		struct {
			tube_sample threshold, value;
		} clip[2];

		tube_sample scale;

	public:
		TwelveAX7_2()
			{ 
				/* transfer polynomial parameters, made with gnuplot::fit() */
				b = -1.08150605597883;
				c = -0.262760944760536;
				d = 0.445770802765903;
			
				static double x[2] = {-.52, +.98};

				for (int i = 0; i < 2; ++i)
					clip[i].threshold = x[i],
					clip[i].value = transfer (x[i]);

				scale = min (fabs (clip[0].threshold), fabs (clip[1].threshold));
			}

		inline tube_sample transfer (tube_sample a)
			{ 
				return a * (b + a * (c + a * d));
			}
			
		inline tube_sample transfer_clip (tube_sample a)
			{
				if (a <= clip[0].threshold)
					return clip[0].value;
				if (a >= clip[1].threshold)
					return clip[1].value;
				return transfer (a);
			}
};

/* third model relies on linear interpolation based on the transfer function
 * as calculated from a spice model.
 */
class TwelveAX7_3
{
	public:
		tube_sample b, c, d;

		struct {
			tube_sample threshold, value;
		} clip[2];

		tube_sample scale;

	public:
		TwelveAX7_3()
			{ 
				static double x[2] = 
					{
						(double) r12AX7::Zero / 
								((double) r12AX7::Samples - (double) r12AX7::Zero),
						1
					};

				for (int i = 0; i < 2; ++i)
					clip[i].threshold = x[i],
					clip[i].value = transfer (x[i]);

				scale = min (fabs (clip[0].threshold), fabs (clip[1].threshold));
			}

		inline tube_sample transfer (tube_sample a)
			{ 
				a = r12AX7::Zero + a * (r12AX7::Samples - r12AX7::Zero);
				if (a <= 0)
					return r12AX7::v2v[0];
				if (a >= r12AX7::Samples - 1)
					return r12AX7::v2v [r12AX7::Samples - 1];

				/* linear interpolation from sampled function */
				register int i = lrintf (a);
				a -= i;
				
				return (r12AX7::v2v [i] * (1.f - a) + r12AX7::v2v [i + 1] * a);
			}
			
		inline tube_sample transfer_clip (tube_sample a)
			{
				return transfer (a);
			}
};

/* experimental */
class NoTwelveAX7
{
	public:
		struct {
			tube_sample threshold, value;
		} clip[2];

		/* amplitude at which clipping starts */
		tube_sample scale;

	public:
		NoTwelveAX7()
			{ 
				static double x[2] = { -1, 1 };

				for (int i = 0; i < 2; ++i)
					clip[i].threshold = x[i],
					clip[i].value = transfer (x[i]);

				scale = min (fabs (clip[0].threshold), fabs (clip[1].threshold));
			}

		inline tube_sample transfer (tube_sample a)
			{ 
				return 0.5469181606780 * (pow (1 - a, 1.5) - 1);
			}
			
		inline tube_sample transfer_clip (tube_sample a)
			{
				if (a <= clip[0].threshold)
					return clip[0].value;
				if (a >= clip[1].threshold)
					return clip[1].value;
				return transfer (a);
			}
};

} /* namespace DSP */

#endif /* _DSP_TWELVE_AX_7_H_ */
