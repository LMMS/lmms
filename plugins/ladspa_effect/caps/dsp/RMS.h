/*
	dsp/RMS.h
	
	Copyright 2004 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	root-mean-square accumulator.

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

#ifndef _DSP_RMS_H_
#define _DSP_RMS_H_

namespace DSP {

class RMS
{
	public:
		d_sample buffer[64];
		int write;
		double sum;

		RMS()
			{
				write = 0;
				reset();
			}

		void reset()
			{
				sum = 0.;
				memset (buffer, 0, sizeof (buffer));
			}

		/* caution: pass in the *squared* sample value */
		void store (d_sample x)
			{
				sum -= buffer[write];
				sum += (buffer[write] = x);
				write = (write + 1) & 63;
			}

		d_sample process (d_sample x)
			{
				store (x);
				return rms();
			}

		d_sample rms()
			{
				/* fabs it before sqrt, just in case ... */
				return sqrt (fabs (sum) / 64);
			}
};

} /* namespace DSP */

#endif /* _DSP_RMS_H_ */
