/*
	Phaser.cc
	
	Copyright 2002-7 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	One simple mono phaser, 6 all-pass lines, the usual controls.

	Another unit in the same vein with the filter modulation controlled by
	a Lorenz fractal.

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

#include "Phaser.h"
#include "Descriptor.h"

template <sample_func_t F>
void
PhaserI::one_cycle (int frames)
{
	d_sample * s = ports[0];

	if (rate != *ports[1])
	{
		rate = getport(1);
		lfo.set_f (max (.001, rate * (double) blocksize), fs, lfo.get_phase());
	}

	double depth = getport(2);
	double spread = 1 + getport(3);
	double fb = getport(4);

	d_sample * dst = ports[5];

	while (frames)
	{
		if (remain == 0) remain = 32;

		int n = min (remain, frames);

		double d = delay.bottom + delay.range * (1. - fabs (lfo.get()));

		for (int j = 5; j >= 0; --j)
		{
			ap[j].set (d);
			d *= spread;
		}
		
		for (int i = 0; i < n; ++i)
		{
			d_sample x = s[i];
			d_sample y = x + y0 * fb + normal;

			for (int j = 5; j >= 0; --j)
				y = ap[j].process (y);
			
			y0 = y;

			F (dst, i, x + y * depth, adding_gain);
		}

		s += n;
		dst += n;
		frames -= n;
		remain -= n;
	}
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
PhaserI::port_info [] =
{
	{
		"in",
		INPUT | AUDIO,
		{BOUNDED, -1, 1}
	}, {
		"rate (Hz)",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_1, 0, 10}
	}, {
		"depth",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_HIGH, 0, 1}
	}, {
		"spread",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0, M_PI}
	}, {
		"feedback",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_HIGH, 0, .999}
	}, {
		"out",
		OUTPUT | AUDIO,
		{0}
	}
};

template <> void
Descriptor<PhaserI>::setup()
{
	UniqueID = 1775;
	Label = "PhaserI";
	Properties = HARD_RT;

	Name = CAPS "PhaserI - Mono phaser";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2002-7";

	/* fill port info and vtable */
	autogen();
}

/* //////////////////////////////////////////////////////////////////////// */

template <sample_func_t F>
void
PhaserII::one_cycle (int frames)
{
	d_sample * s = ports[0];

	lorenz.set_rate (getport(1) * .08);

	double depth = getport(2);
	double spread = 1 + getport(3);
	double fb = getport(4);

	d_sample * dst = ports[5];

	while (frames)
	{
		if (remain == 0) remain = 32;

		int n = min (remain, frames);

		double d = delay.bottom + delay.range * (.3 * lorenz.get());

		for (int j = 5; j >= 0; --j)
		{
			ap[j].set (d);
			d *= spread;
		}
		
		for (int i = 0; i < n; ++i)
		{
			d_sample x = s[i];
			d_sample y = x + y0 * fb + normal;

			for (int j = 5; j >= 0; --j)
				y = ap[j].process (y);
			
			y0 = y;

			F (dst, i, x + y * depth, adding_gain);
		}

		s += n;
		dst += n;
		frames -= n;
		remain -= n;
	}
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
PhaserII::port_info [] =
{
	{
		"in",
		INPUT | AUDIO,
		{BOUNDED, -1, 1}
	}, {
		"rate",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0, 1}
	}, {
		"depth",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_HIGH, 0, 1}
	}, {
		"spread",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0, M_PI * .5}
	}, {
		"feedback",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_HIGH, 0, .999}
	}, {
		"out",
		OUTPUT | AUDIO,
		{0}
	}
};

template <> void
Descriptor<PhaserII>::setup()
{
	UniqueID = 2586;
	Label = "PhaserII";
	Properties = HARD_RT;

	Name = CAPS "PhaserII - Mono phaser modulated by a Lorenz fractal";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2002-7";

	/* fill port info and vtable */
	autogen();
}

