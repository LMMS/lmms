/*
	dsp/BiQuad.h
	
	Copyright 2003-4 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	bi-quad implementation.

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

#ifndef _DSP_BI_QUAD_H_
#define _DSP_BI_QUAD_H_

namespace DSP {

class BiQuad
{
	public:
		/* coefficients */
		d_sample a[3], b[3];

		/* history */
		int h;
		d_sample x[2], y[2];

		BiQuad()
			{
				a[0] = 1;
				a[1] = a[2] = b[0] = b[1] = b[2] = 0;

				reset();
			}
		
		void copy (BiQuad & bq)
			{
				for (int i = 0; i < 3; ++i)
					a[i] = bq.a[i],
					b[i] = bq.b[i];
			}

		void reset()
			{
				h = 0;

				x[0] = x[1] = 
				y[0] = y[1] = 0.;
			}

		/* denormal zapping */
		void flush_0()
			{
				for (int i = 0; i < 2; ++i)
					if (is_denormal (y[i]))
						y[i] = 0;
			}

		inline d_sample process (d_sample s)
			{
				register int z = h;

				register d_sample r = s * a[0];
				
				r += a[1] * x[z];
				r += b[1] * y[z];

				z ^= 1;
				r += a[2] * x[z];
				r += b[2] * y[z];

				y[z] = r;
				x[z] = s;
				
				h = z;

				return r;
			}

		/* additional methods for using the biquad to filter an
		 * upsampled signal with 0 padding */
		inline d_sample process_0_1()
			{
				register int z = h;

				register d_sample r = 0;
				
				r += a[1] * x[z];
				r += b[1] * y[z];

				z ^= 1;
				r += a[2] * x[z];
				r += b[2] * y[z];

				y[z] = r;
				x[z] = 0; 
				
				h = z;

				return r;
			}

		inline d_sample process_0_2()
			{
				register int z = h;

				register d_sample r = 0;
				
				r += b[1] * y[z];

				z ^= 1;
				r += a[2] * x[z];
				r += b[2] * y[z];

				y[z] = r;
				x[z] = 0;
				
				h = z;

				return r;
			}

		inline d_sample process_0_3()
			{
				register int z = h;

				register d_sample r = 0;
				
				r += b[1] * y[z];

				z ^= 1;
				r += b[2] * y[z];

				y[z] = r;
				x[z] = 0;
				
				h = z;

				return r;
			}
};

} /* namespace DSP */

#endif /* _DSP_BI_QUAD_H_ */
