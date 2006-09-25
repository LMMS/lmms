/*
	dsp/sinc.h
	
	Copyright 2003-4 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	computes the sinc function: sin (x * pi) / (x * pi).

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

#ifndef _SINC_H_
#define _SINC_H_

#include "Sine.h"

namespace DSP {
	
/* sample sinc() with step size omega into s[], centered around s + n / 2 */

inline void
sinc (double omega, d_sample * s, int n)
{
	/* initial phase */
	double phi = (n / 2) * -omega;

	Sine sine (omega, phi);
	
	for (int i = 0; i < n; ++i, phi += omega)
	{
		double sin_phi = sine.get();

		if (fabs (phi) < 0.000000001)
			s[i] = 1.;
		else
			s[i] = sin_phi / phi;
	}
}

}; /* namespace DSP */

#endif /* _SINC_H_ */
