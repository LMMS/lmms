/*
	LatFilt.h
	
	Copyright 2006 David Yeh <dtyeh@ccrma.stanford.edu>
	
	Lattice digital filter.
	Assumes order of b = order of a.
	Assumes a0 = 1.

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

#ifndef _DSP_LatFilt_H_
#define _DSP_LatFilt_H_

namespace DSP {

// ORDER is the highest power of s in the transfer function
template <int ORDER>
class LatFilt
{
	public:
		double vcoef[ORDER+1];
		double kcoef[ORDER];
		double state[ORDER];
		double y;

		// fade factors
		double vf[ORDER+1];
		double kf[ORDER];

		
		void reset() 
			{
				for (int i = 0; i < ORDER; i++) {
						state[i] = 0;   // zero state
						vf[i] = 1;      // reset fade factor
						kf[i] = 1;
				}
				vf[ORDER] = 1;
				y = 0;
			}

		void init (double fs)
			{
				reset();
				clearcoefs();
			}

		void clearcoefs() {
				for (int i=0; i< ORDER; i++) {
						vcoef[i] = 0;
						kcoef[i] = 0;
				}
				vcoef[ORDER] = 0;
		}

		d_sample process (d_sample s) {
				double tmp;

				int i = ORDER-1;
				tmp = -kcoef[i]*state[i] + s;
				y = vcoef[i+1]*(state[i] + kcoef[i]*tmp);
						
				for (i = ORDER-2; i >= 0; i--) {
						tmp = -kcoef[i]*state[i] + tmp;
						state[i+1] = kcoef[i]*tmp + state[i];
						y = y + vcoef[i+1]*state[i+1];
				}
				state[0] = tmp;
				y = y + vcoef[0]*tmp;

				return (d_sample) y;
		}

		inline void set_vi(double coef, int i) {
				vcoef[i] = coef;
		}

		inline void set_ki(double coef, int i) {
				kcoef[i] = coef;
		}
};

} /* namespace DSP */

#endif /* _DSP_LatFilt_H_ */
