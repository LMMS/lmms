/*
	HRTF.cc
	
	Copyright 2002-7 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	high-order IIR filtering modeled after HRTF impulse responses

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

#include "basics.h"

#include "HRTF.h"
#include "Descriptor.h"

#include "elev0.h"

/* //////////////////////////////////////////////////////////////////////// */

void
HRTF::init()
{
	h = 0;
}

void
HRTF::set_pan (int p)
{
	n = 31;

	pan = p;

	if (p >= 0)
	{
		left.a = elev0[p].left.a;
		left.b = elev0[p].left.b;
		right.a = elev0[p].right.a;
		right.b = elev0[p].right.b;
	}
	else
	{
		p = -p;
		left.a = elev0[p].right.a;
		left.b = elev0[p].right.b;
		right.a = elev0[p].left.a;
		right.b = elev0[p].left.b;
	}	

	memset (left.y, 0, sizeof (left.y));
	memset (right.y, 0, sizeof (right.y));
}

template <sample_func_t F>
void
HRTF::one_cycle (int frames)
{
	sample_t * s = ports[0];

	int p = (int) getport(1);
	if (p != pan) set_pan (p);

	sample_t * dl = ports[2];
	sample_t * dr = ports[3];

	double l, r;

	for (int i = 0; i < frames; ++i)
	{
		x[h] = l = r = s[i] + normal;
		
		l *= left.a[0];
		r *= right.a[0];

		for (int j = 1, z = h - 1; j < n; --z, ++j)
		{
			z &= 31;
			l += left.a[j] * x[z];
			l += left.b[j] * left.y[z];
			r += right.a[j] * x[z];
			r += right.b[j] * right.y[z];
		}

		left.y[h] = l;
		right.y[h] = r;

		h = (h + 1) & 31;
		
		F (dl, i, l, adding_gain);
		F (dr, i, r, adding_gain);
	}
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
HRTF::port_info [] =
{
	{
		"in",
		INPUT | AUDIO,
		{BOUNDED, -1, 1}
	}, {
		"pan",
		INPUT | CONTROL,
		{BOUNDED | INTEGER | DEFAULT_0, -36, 36}
	}, {
		"out:l",
		OUTPUT | AUDIO,
		{0}
	}, {
		"out:r",
		OUTPUT | AUDIO,
		{0}
	}
};

template <> void
Descriptor<HRTF>::setup()
{
	UniqueID = 1787;
	Label = "HRTF";
	Properties = HARD_RT;

	Name = CAPS "HRTF - Head-related transfer function at elevation 0";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-7";

	/* fill port info and vtable */
	autogen();
}

