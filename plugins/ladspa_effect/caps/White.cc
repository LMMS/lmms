/*
	White.cc
	
	Copyright 2004-7 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	white noise generation.

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

#include "White.h"
#include "Descriptor.h"

template <sample_func_t F>
void
White::one_cycle (int frames)
{
	double g = (gain == *ports[0]) ? 
		1 : pow (getport(0) / gain, 1. / (double) frames);

	sample_t * d = ports[1];

	for (int i = 0; i < frames; ++i)
	{
		F (d, i, gain * white.get(), adding_gain);
		gain *= g;
	}

	gain = getport(0);
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
White::port_info [] =
{
	{
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
Descriptor<White>::setup()
{
	UniqueID = 1785;
	Label = "White";
	Properties = HARD_RT;

	Name = CAPS "White - White noise generator";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-7";

	/* fill port info and vtable */
	autogen();
}

