/*
	dsp/util.h
	
	Copyright 2002-4 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	Common math utility functions.

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

#ifndef _DSP_UTIL_H_
#define _DSP_UTIL_H_

namespace DSP {

inline int next_power_of_2 (int n)
{
	assert (n <= 0x40000000);

	int m = 1;
	
	while (m < n)
		m <<= 1;

	return m;
}

inline bool
isprime (int v)
{
	if (v <= 3)
		return true;
	
	if (!(v & 1))
		return false;

	for (int i = 3; i < (int) sqrt (v) + 1; i += 2)
		if ((v % i) == 0)
			return false;

	return true;
}

inline double 
db2lin (double db)
{
	return pow (10., db * .05);
}

inline double
lin2db (double lin)
{
	return 20. * log10 (lin);
}

} /* namespace DSP */

#endif /* _DSP_UTIL_H_ */
