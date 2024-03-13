/*
	Pan.cc
	
	Copyright 2002-11 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	panorama with width control, 
	stereo image width reduction

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

#include "Pan.h"
#include "Descriptor.h"

void
Pan::init()
{
	delay.init ((int) (.040 * fs));
}

void
Pan::activate()
{ 
	delay.reset();
	tap.reset (400 / fs);
	set_pan (getport (1));
}

inline void
Pan::set_pan (sample_t p)
{
	pan = p;
	
	double phi = (pan + 1) * M_PI * .25;

	gain_l = cos (phi);
	gain_r = sin (phi);
}

template <sample_func_t F>
void
Pan::one_cycle (int frames)
{
	sample_t * s = ports[0];

	if (pan != *ports[1])
		set_pan (getport(1));

	sample_t g = getport(2);
	sample_t 
		width_l = g * gain_r,
		width_r = g * gain_l;

	tap.t = (int) (getport(3) * fs * .001);
	
	bool mono = getport(4);

	sample_t * dl = ports[5];
	sample_t * dr = ports[6];

	sample_t x, xt;

	if (mono) for (int i = 0; i < frames; ++i)
	{
		x = s[i];

		xt = tap.get (delay);

		delay.put (x + normal);

		x = (gain_l * x + gain_r * x + width_l * xt + width_r * xt) * .5;

		F (dl, i, x, adding_gain);
		F (dr, i, x, adding_gain);
		
		normal = -normal;
	}
	else /* stereo */ for (int i = 0; i < frames; ++i)
	{
		x = s[i];

		xt = tap.get (delay);

		delay.put (x + normal);

		F (dl, i, gain_l * x + width_l * xt, adding_gain);
		F (dr, i, gain_r * x + width_r * xt, adding_gain);
		
		normal = -normal;
	}
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
Pan::port_info [] =
{
	{
		"in",
		INPUT | AUDIO,
		{0}
	}, {
		"pan",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, -1, 1}
	}, {
		"width",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0, 0, 1}
	}, {
		"t",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0.1, 40}
	}, {
		"mono",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_0 | INTEGER | TOGGLE, 0, 1}
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
Descriptor<Pan>::setup()
{
	UniqueID = 1788;
	Label = "Pan";
	Properties = HARD_RT;

	Name = CAPS "Pan - Pan and width";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2004-7";

	/* fill port info and vtable */
	autogen();
}

/* //////////////////////////////////////////////////////////////////////// */

void
Narrower::init()
{
}

void
Narrower::activate()
{ 
}

template <sample_func_t F>
void
Narrower::one_cycle (int frames)
{
	sample_t * sl = ports[0];
	sample_t * sr = ports[1];

	if (strength != *ports[2])
		strength = *ports[2];

	sample_t * dl = ports[3];
	sample_t * dr = ports[4];

	double xl, xr, m;
	double dry = 1 - strength, wet = strength;

	for (int i = 0; i < frames; ++i)
	{
		xl = sl[i];
		xr = sr[i];
		
		m = wet * (xl + xr) * .5;
		
		xl *= dry;
		xr *= dry;
		
		xl += m;
		xr += m;

		F (dl, i, xl, adding_gain);
		F (dr, i, xr, adding_gain);
	}
}

/* //////////////////////////////////////////////////////////////////////// */

PortInfo
Narrower::port_info [] =
{
	{
		"in:l",
		INPUT | AUDIO,
		{0}
	}, {
		"in:r",
		INPUT | AUDIO,
		{0}
	}, {
		"strength",
		INPUT | CONTROL,
		{BOUNDED | DEFAULT_LOW, 0, 1}
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
Descriptor<Narrower>::setup()
{
	UniqueID = 2595;
	Label = "Narrower";
	Properties = HARD_RT;

	Name = CAPS "Narrower - Stereo image width reduction";
	Maker = "Tim Goetze <tim@quitte.de>";
	Copyright = "GPL, 2011";

	/* fill port info and vtable */
	autogen();
}

