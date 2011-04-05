/*
	Roessler.cc
	
	Copyright 2002-11 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	the sound of a Roessler attractor.

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

#include "Roessler.h"
#include "Descriptor.h"

void
Roessler::init()
{
	roessler.init (h = .001, frandom());
	gain = 0;
}

template <sample_func_t F>
void
Roessler::one_cycle (int frames)
{
	roessler.set_rate (getport(0));

	double g = (gain == getport(4)) ? 
		1 : pow (getport(4) / gain, 1. / (double) frames);

	sample_t * d = ports[5];

	sample_t x,
			sx = .043 * getport(1), 
			sy = .051 * getport(2), 
			sz = .018 * getport(3);
	
	for (int i = 0; i < frames; ++i)
	{
		roessler.get();

		x = 
				sx * (roessler.get_x() - .515) + 
				sy * (roessler.get_y() + 2.577) + 
				sz * (roessler.get_z() - 2.578);

		F (d, i, gain * x, adding_gain);
		gain *= g;
	}

	gain = getport(4);
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
Roessler::port_info [] =
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
		{BOUNDED | DEFAULT_MID, 0, 1}
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
Descriptor<Roessler>::setup()
{
	UniqueID = 1780;
	Label = "Roessler";
	Properties = HARD_RT;

	Name = CAPS "Roessler - The sound of a Roessler attractor";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-7";

	/* fill port info and vtable */
	autogen();
}

