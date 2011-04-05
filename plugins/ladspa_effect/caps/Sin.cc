/*
	Sin.cc
	
	Copyright 2002-7 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	simple sin() generator.

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

#include "Sin.h"
#include "Descriptor.h"

void
Sin::init()
{
	sin.set_f (f = .005, fs, 0);
	gain = 0;
}

template <sample_func_t F>
void
Sin::one_cycle (int frames)
{
	if (f != *ports[0])
		sin.set_f (f = getport(0), fs, sin.get_phase());

	double g = (gain == *ports[1]) ? 
		1 : pow (getport(1) / gain, 1. / (double) frames);

	sample_t * d = ports[2];

	for (int i = 0; i < frames; ++i)
	{
		F (d, i, gain * sin.get(), adding_gain);
		gain *= g;
	}

	gain = getport(1);
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
Sin::port_info [] =
{
	{
		"f",
		INPUT | CONTROL,
		{BOUNDED | LOG | DEFAULT_100, 0.0001, 20000}
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
Descriptor<Sin>::setup()
{
	UniqueID = 1781;
	Label = "Sin";
	Properties = HARD_RT;

	Name = CAPS "Sin - Sine wave generator";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-7";

	/* fill port info and vtable */
	autogen();
}

