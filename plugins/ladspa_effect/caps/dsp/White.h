/*
  dsp/White.h

	Copyright 2004 Tim Goetze <tim@quitte.de>

	simple white noise generator, based on Jon Dattorro's 3/2002 JAES 
	paper. quite an elegant design; consumes next to no CPU	on a processor
	providing a decent binary shift operator. most of all, no random() calls.

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

#ifndef _DSP_WHITE_H_
#define _DSP_WHITE_H_

namespace DSP {

/* after initializing, call either get() or get_31() to get a sample, don't
 * mix them. (get_31 output goes out of range if called after get()).
 */
class White
{
	public:
		uint32 b;

		White()
			{
				b = 0x1fff7777;
			}

		void init (float f)
			{
				b = (uint32) (f * (float) 0x1fff7777);
			}
		
		d_sample abs()
			{
				return fabs (get());
			}

		/* 32-bit version */
		d_sample get()
			{
#				define BIT(y) ((b << (31 - y)) & 0x80000000)

				b = ((BIT (28) ^ BIT (27) ^ BIT (1) ^ BIT (0))) | (b >> 1);
				return (4.6566128730773926e-10 * (d_sample) b) - 1;

#				undef BIT
			}

		/* 31-bit version, at least 6 instructions less / sample. probably only
		 * pays off on a processor not providing a decent binary shift. */
		d_sample get_31()
			{
#				define BIT(y) ((b << (30 - y)) & 0x40000000)

				b = ((BIT (3) ^ BIT (0))) | (b >> 1);
				return (9.3132257461547852e-10 * (d_sample) b) - 1;

#				undef BIT
			}
};

} /* namespace DSP */

#endif /* _DSP_WHITE_H_ */
