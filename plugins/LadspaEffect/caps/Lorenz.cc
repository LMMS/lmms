/*
	Lorenz.cc
	
	Copyright 2002-11 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	the sound of a Lorenz attractor.

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

#include <stdlib.h>

#include "basics.h"

#include "Lorenz.h"
#include "Descriptor.h"

void
Lorenz::init()
{
	lorenz.init (h = .001, 0.1 * frandom());
	gain = 0;
}

template <sample_func_t F>
void
Lorenz::one_cycle (int frames)
{
	lorenz.set_rate (*ports[0]);

	double g = (gain == *ports[4]) ? 
		1 : pow (getport(4) / gain, 1. / (double) frames);

	sample_t * d = ports[5];

	sample_t x, sx = getport(1), sy = getport(2), sz = getport(3);
	
	for (int i = 0; i < frames; ++i)
	{
		lorenz.step();

		x = sx * lorenz.get_x() + sy * lorenz.get_y() + sz * lorenz.get_z();

		F (d, i, gain * x, adding_gain);
		gain *= g;
	}

	gain = getport(4);
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
Lorenz::port_info [] =
{
	{
		"h",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0, 1}
	}, {
		"x",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_1, 0, 1}
	}, {
		"y",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, 0, 1}
	}, {
		"z",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, 0, 1}
	}, {
		"volume",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_MID, MIN_GAIN, 1}
	}, {
		"out",
		OUTPUT | AUDIO,
		{0}
	}
};

template <> void
Descriptor<Lorenz>::setup()
{
	UniqueID = 1774;
	Label = "Lorenz";
	Properties = HARD_RT;

	Name = CAPS "Lorenz - The sound of a Lorenz attractor";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-7";

	/* fill port info and vtable */
	autogen();
}

